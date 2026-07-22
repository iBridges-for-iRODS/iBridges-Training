# Working with IrodsPath

The Python API uses `IrodsPath` objects to represent locations inside an iRODS server.  
They behave similarly to `pathlib` paths, but operate on the iRODS namespace.  
All interactions with collections and data objects begin with an `IrodsPath`.

An `IrodsPath` always belongs to a `Session` and resolves either absolute or relative iRODS paths.

### Creating an IrodsPath

Absolute paths must start with a slash:

```python
from ibridges.path import IrodsPath

p = IrodsPath(session, "/tempZone/home/christine/demo")
print(p)
```

The operator `/` extends paths:

```python
sub = p / "experiment1"
print(sub)
```

But let us first explore the home and the current working directory a bit more.

### Home and current working directory

The Session defines two important locations:

- session.home  
- session.cwd  

The home is your default working directory in iRODS.  
It can be set in three ways:

1. In irods_environment.json  
2. When creating the Session  
3. By assigning session.home manually

If not set, it defaults to `/zone/home/username`.

The cwd is optional and defaults to the home.  
The dot symbol refers to the cwd:

```
IrodsPath(session, ".")
```

The tilde symbol refers to the home:

```
IrodsPath(session, "~")
```

You can set your `demo` collection as current working directory:

```
session.cwd = IrodsPath(session, "~", "demo")
print("cwd:", session.cwd)
print("home:", session.home)
```

Relative paths are resolved against the session’s cwd:

```python
p = IrodsPath(session, "subcoll")
print(p)
```

> You can create instances of  `IrodsPath`, however, that does not mean that they are present on the iRODS server.

Let's have a look at how to check whether a path exists and how to create a new collection.

## Checking existence

```python
print(p)
print("Exists:", p.exists())
print("Is a collection:", p.collection_exists())
print("Is a data object:", p.dataobject_exists())
```

## Creating collections

Creating an IrodsPath does not create anything on the server.  
To create a collection:

```python
coll = p.create_collection()
```

This creates the entire path if needed.

```
print("Exists:", p.exists())
print("Is a collection:", p.collection_exists())
print("Is a data object:", p.dataobject_exists())
```

And it returns the collection (not the path). We will investigate that later when we work with data.

### Renaming or moving

```python
print(p)
q = p.rename("new_name")
print(q)
```

The old variable `p` still contains our old path. But it does not exist any longer:

```
p.exists()
```

The `rename` function returns the new `IrodsPath` which does exist:

```
q.exists()
```

You can also use the `rename` function to move a collection or data object to a different location:

```python
target = IrodsPath(session, "/tempZone/home/christine/archive/my_experiment")
new_p = q.rename(target)

print(new_p)
print("Is a collection:", new_p.collection_exists())
```

We have moved our collection `new_name` to a completely new branch in the tree of iRODS.

### Removing collections or data objects

Let us remove the whole new `archive` collection.
We will use the `parent` of the path to navigate one level up and then apply the `remove` function to this collection:

```python
new_p.parent.remove()
new_p.collection_exists()
new_p.parent.collection_exists()
```

The IrodsPath object remains, but the server location is deleted.

### Inspecting properties

```python
p.absolute()
p.parts
p.parent
p.size
```

These help you understand the structure and contents of the path.

## Exercises

### Exercise 1: Basic path handling

- Create a session using interactive_auth  
- Construct an IrodsPath pointing to your home using "~"  
- Verify that the path exists  
- Print its parts and parent

### Exercise 2: Creating and navigating collections

- Verify the existence of `session.home / "demo"` and create it if necessary   
- Extend the path with "experiment1" using `/`  
- Create the subcollection

### Exercise 3: Renaming and moving

- Rename experiment1 to experimentA  
- Move experimentA into a new collection session.home / "archive"  
- Verify the new location exists

### Exercise 4: Removing collections

- Remove the archive/experimentA collection  
