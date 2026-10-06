# Setting up an iRODS training server

A step-by-step guide for people who have not used Ansible before. You do not need to
understand Ansible in depth: you will install it, tell it which machine to use, and run
a few commands.

For a description of what the package contains, see [README.md](README.md).

## Contents

1. [How it works](#1-how-it-works)
2. [What you need](#2-what-you-need)
3. [Prepare your computer](#3-prepare-your-computer)
4. [Tell Ansible about your VM](#4-tell-ansible-about-your-vm)
5. [Build the training server](#5-build-the-training-server)
6. [Set the passwords](#6-set-the-passwords)
7. [Check that everything works](#7-check-that-everything-works)
8. [Change what gets installed](#8-change-what-gets-installed)
9. [Protect the passwords](#9-protect-the-passwords)
10. [Between two courses: reset the users](#10-between-two-courses-reset-the-users)
11. [Tear everything down](#11-tear-everything-down)
12. [Troubleshooting](#12-troubleshooting)
13. [Glossary](#13-glossary)

## 1. How it works

Ansible runs on **your computer** and configures **another machine** (the VM) over SSH.
You describe the wanted end state in text files; Ansible makes the VM match it. Nothing
has to be installed on the VM beforehand, apart from Python (already present on Ubuntu)
and an SSH server.

You will meet four words:

- **Control node**: the computer you type commands on.
- **Inventory** (`inventory.ini`): the list of machines. Here, just one VM.
- **Playbook** (`*.yml` in the project root): a recipe you run, such as
  `setup_training.yml`.
- **Role** (folders under `roles/`): a reusable part of a recipe, such as "install iRODS".

A playbook run prints one line per task: `ok` (already fine), `changed` (Ansible did
something), `skipped`, or `failed`. Playbooks can be run as often as you like. They only
change what is not yet in the wanted state.

## 2. What you need

**A VM** with

- Ubuntu 24.04 (22.04 also works), 64-bit Intel/AMD (amd64)
- an IP address or name you can reach with SSH
- a user that can use `sudo`
- enough disk space for the training data, and for the extra storage path
  (default `/data/10gb`) if you want the data on a separate disk

**On the network**: the VM must accept SSH from your computer. Clients that use iRODS
must be able to reach ports **1247** and **20000-20199**. On OpenStack and similar
platforms, open these in the *security group* of the VM.

**Your computer**: Linux or macOS with Python 3. (On Windows, use WSL.)

## 3. Prepare your computer

### 3.1 Install Ansible in its own environment


Open a terminal in the  folder `training-server` (the one containing `inventory.ini`):

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install ansible
ansible-galaxy collection install community.postgresql
```

The first command creates a private Python environment in the folder `.venv`, so
nothing is installed system-wide. **Every time you open a new terminal**, run
`source .venv/bin/activate` again; your prompt then starts with `(.venv)`. Leave the
environment with `deactivate`.

Check the installation:

```bash
ansible --version
```

### 3.2 Make sure you can SSH into the VM

```bash
ssh YOURUSER@VM_ADDRESS
```

If this asks for a password every time or fails, set up an ssh key first:

```bash
ssh-keygen -t ed25519            # only if you do not have a key yet
ssh-copy-id YOURUSER@VM_ADDRESS
```

Then check that your user can use sudo **without a password**:

```bash
ssh YOURUSER@VM_ADDRESS "sudo -n true && echo sudo works"
```

If this prints `a password is required`, you have two options: ask whoever manages the VM to allow passwordless sudo for your user, or add `--ask-become-pass` to every `ansible-playbook` command in this guide and type the sudo password when asked.

## 4. Tell Ansible about your VM

Open `inventory.ini` and put in your VM:

```ini
[irods]
irods-vm ansible_host=VM_ADDRESS ansible_user=YOURUSER
```

- `irods-vm` is just a label.
- `VM_ADDRESS` is the IP address or DNS name you use for SSH.
- `YOURUSER` is your login on the VM.

Test the connection:

```bash
ansible -i inventory.ini irods -m ping
```

You should see `"ping": "pong"`. If not, fix SSH first (section 3.2) before going on.

## 5. Build the training server

Before you run the script please adjust the path in the variable `training_resources` in `roles/irods_training/defaults/main.yml` to a location which exists on your VM.

### 5.1 What will be created (optional)

The defaults create 30 users (`irods1` ... `irods30`), an administrator
`training_admin`, two groups, one extra storage resource and three sets of example data
(from the `data/` folder). [Section 8](#8-change-what-gets-installed) explains how to change this.


Check that the data is where the playbook expects it:

```bash
ls data/ascii.txt data/game data/my_books data/ibridges_metadata_*.json
```

### 5.2 Run the playbook

```bash
ansible-playbook -i inventory.ini setup_training.yml
```

This takes a few minutes. Ansible installs PostgreSQL and iRODS, runs the iRODS setup,
creates the resources, users and groups, installs the rules and uploads the data.

**Success looks like this** at the end of the output:

```
PLAY RECAP ****
irods-vm : ok=…  changed=…  unreachable=0  failed=0  skipped=…
```

`failed=0` and `unreachable=0` mean it worked. If something failed, jump to
[Troubleshooting](#12-troubleshooting).

If you only want a plain iRODS server without the training setup, run
`install_irods.yml` instead. You do not need to run both.

## 6. Set the passwords

The playbook creates the users **without passwords**. Nobody can log in until you set them. Passwords are set on the VM, as the iRODS service account:

```bash
ssh YOURUSER@VM_ADDRESS
```

Set one password for the administrator:

```bash
sudo su - irods
iadmin moduser training_admin password 'CHOOSE-A-PASSWORD'
```

Set the same starting password for all 30 training users:

```bash
for i in $(seq 1 30); do
  iadmin moduser irods$i password 'CHOOSE-A-PASSWORD'
done
```

Or choose a different password per user:

```bash
iadmin moduser irods7 password 'something-else'
```

Notes:

- Re-running the playbook never resets passwords.

## 7. Check that everything works

Run these on the VM (`ssh YOURUSER@VM_ADDRESS`).

**The server is running:**

```bash
systemctl status irods
```

It should say `active (running)`.

**Resources, users and groups exist:**

```bash
sudo su - irods
ilsresc                                # demoResc and TrainingResc
iadmin lg training | wc -l             # the group members (31)
iadmin lg datastewards                 # training_admin#tempZone
```

**The data is there:**

```bash
ils -lr /tempZone/home/training/my_books
ils -lr /tempZone/home/training_admin/game
ils -lr /tempZone/home/training_admin/ascii_art
```

**A training user can log in** (after setting a password in step 6). 
Make sure you are logged out as the user `irods`!!!

```
exit
```

Create a small settings file and log in:
	
```bash
mkdir -p ~/.irods
cat > ~/.irods/irods_environment.json <<'EOF'
{
  "irods_host": "localhost",
  "irods_port": 1247,
  "irods_user_name": "irods1",
  "irods_zone_name": "tempZone"
}
EOF
iinit                      
# type the password
ils /tempZone/home/training/my_books
```

### Connecting from the participants' computers

Participants need the address of the VM instead of `localhost`:

```json
{
  "irods_host": "VM_ADDRESS",
  "irods_port": 1247,
  "irods_user_name": "irods7",
  "irods_zone_name": "tempZone"
}
```

Stored as `~/.irods/irods_environment.json` for the iCommands, or entered in a client such as iBridges. Their computers must be able to reach ports 1247 and 20000-20199 on the VM.

## 8. Change what gets installed

All settings are in `defaults/main.yml` files inside the roles. Edit the file, save, and
run the playbook again. Existing parts are left alone; only the new or changed part is
done.

### `roles/irods/defaults/main.yml`

| Variable | Default | Meaning |
|---|---|---|
| `irods_version` | `4.3.5` | iRODS version to install |
| `irods_release` | detected | Ubuntu release name used for the apt repository |
| `irods_db_name` / `irods_db_user` | `ICAT` / `irods` | Catalog database and its user |
| `irods_db_password` | `testpassword` | **Change before real use** |
| `irods_zone` | `tempZone` | Zone name |
| `irods_admin_user` / `irods_admin_password` | `rods` / `rods` | **Change before real use** |
| `irods_zone_key`, `irods_negotiation_key`, `irods_control_plane_key` | sample values | **Change before real use**; the two negotiation/control keys must be exactly 32 bytes |

### `roles/irods_training/defaults/main.yml`

| Variable | Default | Meaning |
|---|---|---|
| `training_resources` | `TrainingResc` on `/data/10gb` | List of extra resources (`name`, `vault`) |
| `training_user_prefix`, `training_user_count` | `irods`, `30` | Users `irods1` ... `irods30` |
| `training_admin_user` | `training_admin` | The `rodsadmin` account |
| `training_group` | `training` | Group with all training users |
| `training_group_extra_members` | the admin user | Extra members of that group |
| `training_datasteward_group` | `datastewards` | Group for data stewards |
| `training_datastewards_members` | the admin user | Use `[]` for an empty group |
| `training_rulebase_name` | `hooks` | Rule file `/etc/irods/<name>.re` |
| `training_data_resource` | `TrainingResc` | Resource the data is uploaded to (`""` = default) |
| `training_datasets` | three datasets | List of data to upload, see below |

A dataset has these fields:

| Field | Meaning |
|---|---|
| `name` | Label used in task output and for the temporary staging folder |
| `src` | Folder (or single file) on your computer |
| `metadata` | iBridges metadata export (`.json`), or `""` for none |
| `dest` | Destination collection in iRODS |
| `owner` | User that gets `own` permission |
| `group_permission` | `read`, `write`, `own`, or `""`; applied to the `training` group |
| `type` | `file` if `src` is a single file; then `dest` is the collection the file goes into |

### More or fewer users

`roles/irods_training/defaults/main.yml`:

```yaml
training_user_count: 40
```

New users are added; existing ones keep their passwords. (Reducing the number does not
delete users.)

### A different name or path for the extra storage

```yaml
training_resources:
  - name: TrainingResc
    vault: /data/10gb
```

Rules for the path:

- It must already be mounted if it is a separate disk. If it is not, the playbook creates
  a plain folder on the system disk.
- The playbook makes the `irods` account its owner.
- Keep `irods_cleanup_extra_vaults` in `roles/irods_cleanup/defaults/main.yml` in step,
  or the cleanup will not find the data.

You can list more than one resource.

### Your own training data

1. Copy the files into the `data/` folder, for example `data/my_course/`.
2. Optionally export the metadata from iBridges to `data/ibridges_metadata_my_course.json`.
3. Add an entry to `training_datasets`:

```yaml
training_datasets:
  - name: my_course
    src: "{{ playbook_dir }}/data/my_course"
    metadata: "{{ playbook_dir }}/data/ibridges_metadata_my_course.json"
    dest: "/{{ irods_zone }}/home/training_admin/my_course"
    owner: "{{ training_admin_user }}"
    group_permission: read
```

For a single file, add `type: file`; then `dest` is the collection that the file is put
into. Use `metadata: ""` if there is no metadata file, and `group_permission: ""` if the
training group should not see the data.
The metadata file is an ibridges metadata export file.

Data that already exists in iRODS is not uploaded again. To replace a dataset, remove
it first:

```bash
sudo -iu irods irm -rf /tempZone/home/training_admin/my_course
```

and run the playbook again. New netadata is always added, also when the data already exists.

## 9. Protect the passwords

The defaults contain example passwords and keys (`testpassword`, `rods`, sample keys).
That is fine for a quick test; for a real course, replace them. They are used **only
during the first setup**, so set them before running the playbook the first time.

Create an encrypted file with your own values:

```bash
mkdir -p group_vars/irods
ansible-vault create group_vars/irods/vault.yml
```

Ansible asks for a vault password and opens an editor. Put in:

```yaml
irods_db_password: "a-long-random-password"
irods_admin_password: "another-long-random-password"
irods_zone_key: "SOME_ZONE_KEY"
irods_negotiation_key: "exactly-32-bytes-of-characters!!"
irods_control_plane_key: "another-32-bytes-of-characters!!"
```

The two keys must be **exactly 32 characters**. Check with `printf '%s' 'your-key' | wc -c`.

The folder name `group_vars/irods` matches the group `[irods]` in the inventory, so
the values are picked up automatically and override the defaults. Run the playbook with
the vault password:

```bash
ansible-playbook -i inventory.ini setup_training.yml --ask-vault-pass
```

Do not commit the vault password. The encrypted file itself is safe to keep in git.

## 10. Between two courses: reset the users

To empty the training users' home collections and invalidate their passwords:

```bash
ansible-playbook -i inventory.ini reset_users.yml -e reset_users_confirm=true
```

What it does for each of `irods1` ... `irods30`:

- gives the iRODS administrator access to the home collection (an administrator has no
  automatic access to other users' homes)
- deletes everything in the home collection permanently, including the trash
- sets a random password nobody knows

What it leaves alone: the users and groups themselves, the shared example data
(`my_books`, `game`, `ascii_art`), and `training_admin`.

Variations:

```bash
# only the data, keep the passwords
ansible-playbook -i inventory.ini reset_users.yml -e reset_users_confirm=true -e reset_users_reset_passwords=false

# only some users
ansible-playbook -i inventory.ini reset_users.yml -e reset_users_confirm=true -e '{"reset_users_names":["irods1","irods2"]}'
```

After a reset, set new passwords as in [section 6](#6-set-the-passwords).

## 11. Tear everything down

To remove iRODS and its data from the VM:

```bash
ansible-playbook -i inventory.ini cleanup.yml -e irods_cleanup_confirm=true
```

**This permanently deletes**

- the iRODS server, all packages, the `irods` account, `/etc/irods` and `/var/lib/irods`
  (including the default vault)
- the `ICAT` catalog database and the database user
- the `home` and `trash` folders in the extra storage path (default `/data/10gb`), and
  **only** those; everything else in that folder stays
- the iRODS apt repository

**It keeps** PostgreSQL itself, the `/data/10gb` folder, and general tools such as
`acl`. Extras, if you want them:

```bash
# also remove PostgreSQL: WARNING, this deletes ALL databases on the machine
-e irods_cleanup_remove_postgresql=true

# also remove this machine's line from /etc/hosts
-e irods_cleanup_remove_hosts_entry=true

# keep the data in the extra storage path
-e irods_cleanup_empty_extra_vaults=false
```

You can run the cleanup twice; the second run finds nothing to do. After a cleanup,
`setup_training.yml` builds a fresh server again.

## 12. Troubleshooting

When a task fails, Ansible prints the task name and an error. Read the **first** error.
Some tasks hide their output on purpose because it contains passwords (`no_log`); to see
the real error, run the failing command by hand on the VM.

| Message or symptom | Cause and fix |
|---|---|
| `ansible-galaxy requires resolvelib` | Your Ansible is mismatched with its Python packages. Use the virtual environment from section 3.1. |
| `Permission denied (publickey)` | The SSH key is not on the VM. Run `ssh-copy-id YOURUSER@VM_ADDRESS`. |
| `Missing sudo password` / `a password is required` | Add `--ask-become-pass` to the command, or ask for passwordless sudo. |
| `Failed to set permissions on the temporary files … chmod: invalid mode: 'A+user:postgres…'` | The `acl` package is missing on the VM. The role installs it; make sure you are running a current copy of the role. |
| `no available installation candidate for irods-…` | The apt repository is wrong for the VM's Ubuntu release or was not refreshed. On the VM: `apt-cache policy irods-server` should list versions ending in `~noble` (or `~jammy` for Ubuntu 22.04). |
| `The hostname (…) must resolve to the local machine` | The VM's DNS name points to a public address that is not on the VM (typical for a floating IP). The role adds an `/etc/hosts` entry that fixes this; check that the task before the setup ran, and `getent hosts $(hostname -f)` on the VM should show the VM's own address. |
| `groupadd: ' ' is not a valid group name` | The setup answer file contains a line with a space instead of an empty line. `grep -n '^[[:space:]]\+$' roles/irods/templates/setup.input.j2` should print nothing. On macOS remove them with `perl -pi -e 's/^\s+$/\n/' roles/irods/templates/setup.input.j2`. |
| `CAT_NO_ACCESS_PERMISSION` during an upload | The service account has no write access to the target collection. The role handles this with `ichmod -M own`; on the VM, `sudo -iu irods ils -A -d /tempZone/home/training` shows who may write. |
| `CATALOG_ALREADY_HAS_ITEM_BY_THAT_NAME` | The item (resource, user, group) already exists but was not recognised. Check that the role files are current, and look at `iadmin lu`, `ilsresc`, `iadmin lg`. |
| A resource is created but puts to it fail | Its location must be the VM's full host name as iRODS knows it. Compare `ilsresc -l demoResc` and `ilsresc -l TrainingResc`; the `location` lines must match. |
| Odd metadata such as attribute `g` or `C` | The metadata JSON is malformed (see section 8). Fix the file, remove the dataset in iRODS with `irm -rf`, and run the playbook again. |
| Port 1247 is not reachable from other computers | Open ports 1247 and 20000-20199 in the VM's firewall or the cloud security group. |
| The `/etc/hosts` line disappears after a reboot | cloud-init rewrote the file. iRODS keeps working; re-run the playbook if you need to run the setup again. |

### Getting more information

On the VM:

```bash
systemctl status irods
sudo journalctl -u irods -f
```

On your computer, add `-v` (or `-vvv`) to a playbook command for more detail.

## 13. Glossary

| Term | Meaning |
|---|---|
| **iRODS** | Data management system; stores files with metadata and permissions |
| **Zone** | One iRODS installation's namespace; here `tempZone` |
| **Resource** | A storage location iRODS can put files on (`demoResc`, `TrainingResc`) |
| **Vault** | The directory on disk that holds the files of a resource |
| **Collection / data object** | iRODS words for folder / file |
| **rodsadmin / rodsuser** | Administrator account / normal user account |
| **Catalog (ICAT)** | The PostgreSQL database in which iRODS keeps all information about files and users |
| **Control node** | Your computer, where you run Ansible |
| **Vault (Ansible)** | An encrypted file for passwords; unrelated to an iRODS vault |
