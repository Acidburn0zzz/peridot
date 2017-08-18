Verb Template
=============
> Status: DRAFT
>
> NOTE: There is a proposal to "publish" these not to Fuchsia packages, but
> rather somewhere within the build tree, since they are strictly only necessary
> at build time, not at runtime.

A **verb** represents a high-level operation that a `Module` can perform. The
operations are described independently of the specific data types they operate
on or the modules that implement them.

A **verb template** defines the *call signature* and *semantics* for a single
*verb*. Verbs are labels given to high-level operations in Fuchsia that may be
performed by `Modules` (**TODO: link**) with a given set of data.

Example verbs could include `Navigate`, `Preview` or `Summarize`.

Modules implement verbs and must adhere to the call signature and behavior
described in the verb template.

There is no prescribed ontology of verbs. Anyone may publish a verb template and
begin to utilize the verb defined within. For this reason, verbs are
unambiguously identified by both the package in which they were published as
well as the verb itself.

## The `meta/verb_template` file

Verb templates are defined in the `verb_template` metadata file (**TODO:
link**) of a Fuchsia package. The content of the metadata file is a JSON array
of dictionaries. The full JSON schema, for reference, can be [found
here](../src/package_manager/metadata_schemas/verb_template.json).

Below is an example, followed by a detailed breakdown of the properties.

## Example

The following `meta/verb_template` file defines three verbs: `Preview`,
`Navigate` and `Pick`.

`Preview` accepts a single required argument (called a "noun"), which
represents the `Entity` that will be previewed.

`Navigate` accepts two nouns, one of which (the *destination*) is required.

`Pick` accepts an optional source (from which to pick an item), and returns a
single "picked" entity.

```javascript
[
  {
    "name": "Preview",
    "nouns": [
      {
        "name": "entityToPreview",
        "direction": "input",
        "required": true
      }
    ],
    "doc": "docs/preview.md"
    // The above doc file contains:
    // Renders a static or interactive preview of |entityToPreview|.
    // May include an affordance which when selected displays a
    // larger preview, or launches a new Module / experience.
  },
  {
    "name": "Navigate",
    "nouns": [
      {
        "name": "start",
        "direction": "input"
      },
      {
        "name": "destination",
        "direction": "input",
        "required": true
      }
    ],
    "doc": "docs/navigate.md"
  },
  {
    "name": "Pick",
    "nouns": [
      {
        "name": "source",
        "direction": "input"
      },
      {
        "name": "picked",
        "direction": "output"
      }
    ],
    "doc": "docs/pick.md"
  }
  // TODO: Add an example that uses input/output direction on a noun.
]
```

## Details

Let's go through the properties that make up a verb template.

### verb name

```javascript
"name": "Preview",
```
> TODO(thatguy): This is a machine-readable name, not a human-readable name. Add
> something for humans that supports localization.

This is the verb's name, unique within the `verb_template` file. Verb names are
constrained to the following characters: `[a-zA-Z0-9_]`. It is convention to use
camel-case with the first letter capitalized.

Verb names allow `Module` [metadata files](module.md) to reference this verb
template when they declare that they provide an implementation. When
referencing, the developer will include both the package containing this
`meta/verb_template` file, as well as the `name`.

### nouns

```javascript
"nouns": [
  {
    "name": "entityToPreivew",
    "direction": "input",
    "required": true
  }
],
```

> TODO: There is no way to specify that a noun argument should be a list of
> Entities.  This may be possible simply by adding a `is_list` property.

Nouns give names to individual or sets of [`Entity`](../entity.md) references
passed between `Modules` at runtime.

Each noun declared in a `verb` definition must have the following properties:

* `name`: limited to the following characters: `[a-zA-Z0-9_]`
* `direction`: any of `input`, `output` and `input/output`.
  - `input`: only the target `Module`'s parent may set the `Entity`
    reference(s) value for this noun.
  - `output`: only the target `Module` may set the `Entity` reference(s).
  - `input/output`: either may set the `Entity` reference(s).
* `required`: `true` or `false`. Default: `false`

Note that nouns do not specify `Entity` types here. These are
defined at the time of declaring an implementation of this `verb`
by a `Module`, which is done in a [`Module` metadata file](module.md).


### JSON parameters

TODO

### return JSON

TODO

### documentation

```javascript
"doc": "docs/preview.md",
```
`doc` is a path to a Markdown file, relative to the root of the package.

The documentation should identify expected behavior for `Modules` that
implement this `verb` and what role each `noun` plays in that behavior.
