# iBridges Training
<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/80x15.png" /></a>
[![Built with Quarto](https://img.shields.io/badge/Built%20with-Quarto-3C6EB4.svg)](https://quarto.org)
[![Follow the training](https://img.shields.io/badge/read-the%20book-yellow.svg)](https://ibridges-for-irods.github.io/iBridges-Training/)

This repository contains the materials for the iBridges training. It

- introduces RDM concepts and how they are implemented in iRODS
- takes beginners and non-technical researchers through the GUI and CLI
- introduces the basics of the iBridges Python API to scientific programmers

## iRODS training server (Ansible)

The repository also contains ansible scripts to build a training instance which deploys the policies discussed in the training, it sets up the users and the training data.

If you are new to Ansible, start with [USAGE.md](training-server/USAGE.md). This file describes what
the package contains and how it is put together.

### What you get
**Plain server** (`install_irods.yml`)

- PostgreSQL with the `ICAT` catalog database
- iRODS server (default version 4.3.5), iCommands and the PostgreSQL plugin from the
  official RENCI apt repository
- A systemd service (`irods`) that starts the server at boot
- Zone `tempZone`, administrator `rods`, default resource `demoResc`

**Training server** (`setup_training.yml`): the plain server plus

- An extra resource `TrainingResc` on a separate path (default `/data/10gb`); the path
  is made writable for the `irods` service account
- 30 users `irods1` ... `irods30` (type `rodsuser`, **no password**, set by hand later)
- An administrator `training_admin` (type `rodsadmin`, no password)
- Group `training` with all 30 users and `training_admin`
- Group `datastewards` with `training_admin`
- Event rules in `/etc/irods/hooks.re`, registered in `server_config.json`
- Training data uploaded to iRODS with its metadata, owned by `training_admin`

**Maintenance playbooks**

- `reset_users.yml`: empties the home collections of the training users and
  invalidates their passwords (between two courses)
- `cleanup.yml`: removes iRODS from the machine again

### Requirements

| Where | What |
|---|---|
| Your computer ("control node") | Linux or macOS, Python 3, Ansible (recent `ansible-core`), the `community.postgresql` collection |
| The VM ("target") | Ubuntu 24.04 (or 22.04), amd64, SSH access, a user with sudo, Python 3 |
| Network | SSH (22) from you to the VM; iRODS ports 1247 and 20000-20199 for clients |

## Quick start

```bash
python3 -m venv .venv && source .venv/bin/activate
pip install ansible
ansible-galaxy collection install community.postgresql

# put the VM address in inventory.ini, then:
ansible -i inventory.ini irods -m ping
ansible-playbook -i inventory.ini setup_training.yml
```

Then set the user passwords by hand (see [USAGE.md](USAGE.md#6-set-the-passwords)).

## Playbooks

| Playbook | Purpose | Safety switch |
|---|---|---|
| `install_irods.yml` | Plain iRODS server only | none |
| `setup_training.yml` | Plain server + everything for the training | none |
| `reset_users.yml` | Delete the data of the training users, invalidate passwords | `-e reset_users_confirm=true` |
| `cleanup.yml` | Remove iRODS, its database, users and data from the machine | `-e irods_cleanup_confirm=true` |

`setup_training.yml` does not need `install_irods.yml` to be run first: the training
role depends on the plain role and pulls it in.

All playbooks are safe to run again. Existing users, groups, resources and data are
detected and left alone, so a second run should report `changed=0`.

## Roles

### `irods`

Installs PostgreSQL and iRODS and runs the one-time setup. Main steps, in order:

1. Install `acl` (needed by Ansible to run tasks as the `postgres` user)
2. Install PostgreSQL, create the `irods` database user and the `ICAT` database
3. Add the RENCI apt repository for the release of the VM (`noble` or `jammy`)
4. Install `irods-server`, `irods-runtime`, `irods-icommands` and
   `irods-database-plugin-postgres`, all pinned to the same version
5. Make the VM's own hostname resolve to a local address in `/etc/hosts`
   (iRODS setup refuses to run otherwise)
6. Run `setup_irods.py`, answers come from `templates/setup.input.j2`
7. Install and start the `irods` systemd service

### `irods_training`

Depends on `irods`. Creates the resources, users, groups, installs the rules and uploads
the datasets. Uploads run as the iRODS service account, so the users need no password
for the setup to work.

### `irods_reset_users`

For each training user: takes ownership of the home collection with `ichmod -M`
(an iRODS administrator has no automatic access to user homes), removes everything in
it with `irm -rf`, empties the trash, and sets a random password nobody knows.

### `irods_cleanup`

Stops the service, drops the `ICAT` database and the `irods` database user, purges all
`irods-*` packages, deletes `/etc/irods`, `/var/lib/irods` and the `irods` user, removes
the apt repository, and removes the `home` and `trash` folders from the extra resource
vaults. PostgreSQL itself is kept unless you ask for it to be removed.

## Configuration

Settings live in the `defaults/main.yml` of each role. Do not edit them for secrets: use
an Ansible vault (see [USAGE.md](training-server/USAGE.md#9-protect-the-passwords)).


## Known limitations

- Changing `irods_version` re-installs packages, but the one-time setup only runs once
  (it is skipped when `/etc/irods/server_config.json` exists). A major upgrade such as
  5.x needs its own procedure and has not been tested; iRODS 5 setup prompts may differ
  from the answer file in `templates/setup.input.j2`.
- Database passwords and keys are read once, at setup time. Changing them in the
  variables later does not change an existing installation.
- cloud-init may rewrite `/etc/hosts` after a reboot. A running iRODS is not affected;
  re-run the playbook before running a new setup.
- Tested on Ubuntu 24.04 with iRODS 4.3.x.

