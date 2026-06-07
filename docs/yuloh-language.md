# Yuloh language reference

Yuloh is Sampan's document language. A `.yl` file describes a rendered
document as a tree of nodes, literal properties, and text. One file is one
UTF-8 document. Imports, scripting, and runtime behaviour are not supported yet.

## Source text

Whitespace separates tokens but is otherwise insignificant. Comments use `//`
through a line end or non-nesting `/* ... */`. Identifiers begin with a letter
or `_`; later characters may also be digits, `-`, or `_`.

Only `true` and `false` are reserved words.

## Literal values

| Value | Examples |
|---|---|
| Integer | `0`, `42`, `-3` |
| Float | `3.14`, `-0.5` |
| Length | `20px` |
| Color | `#eef`, `#ff8800` |
| String | `"Hello"` |
| Boolean | `true`, `false` |

Only `px` lengths and three- or six-digit opaque colors are supported. Strings
support `\n`, `\t`, `\\`, `\"`, `\{`, and `\}` escapes. Strings are plain
text and interpolation is not supported.

## Documents and nodes

A document contains one root node. A node has a tag and a body. The tag is
written using identifier syntax. The body may contain literal properties,
bare string text children, and nested nodes. Items appear in rendered order.

```yl
page {
  title: "Hello"
  background: #ffffff

  stack {
    padding: 20px
    heading { content: "Hello, world" }
    text { content: "A static Yuloh document." }
  }
}
```

### Built-in nodes

| Node | Role | Node-specific properties |
|---|---|---|
| `page` | Document root | `title: String`, `align: String`, `justify: String` |
| `stack` | Vertical container | `gap: Length`, `align: String`, `justify: String` |
| `row` | Horizontal container | `gap: Length`, `align: String`, `justify: String` |
| `box` | Plain block box | `align: String`, `justify: String` |
| `text` | Text content | `content: String`, `color: Color`, `size: Length`, `weight: Integer` |
| `heading` | Text with heading defaults | `content: String`, `color: Color`, `size: Length`, `level: Integer` |
| `button` | Static block | `align: String`, `justify: String` |
| `spacer` | Fixed empty gap | `size: Length` |

`page`, `stack`, `row`, and `box` are containers and may contain any number of
child nodes. `text`, `heading`, `button`, and `spacer` are leaf nodes and cannot
contain child nodes. The document itself still contains exactly one root
`page` node.

Every built-in node accepts these universal layout properties:

- `width`, `height`, `padding`, `padding-top`, `padding-right`,
  `padding-bottom`, `padding-left`, `margin`, `margin-top`, `margin-right`,
  `margin-bottom`, `margin-left`, and `border-width`: `Length`
- `border-color` and `background`: `Color`

The root node must be `page`, and a `page` cannot be nested. Property names
must be valid for their node, values must have the documented type, and a
property may appear at most once on a node.

### Layout behavior

`page`, `stack`, and `box` place their text and child nodes vertically in
source order. `stack` inserts its `gap` between adjacent items.

`row` places its text and child nodes from left to right. If the next item plus
the preceding `gap` would exceed the row's content width, that item starts a
new line. The same `gap` is used horizontally between items and vertically
between wrapped lines. An item wider than an empty line remains on that line
and may overflow it.

`align` controls the cross axis and `justify` controls the main axis. The axes
depend on the node's flow direction:

| Flow owner | Main axis | Cross axis |
|---|---|---|
| `row` | Horizontal | Vertical |
| `page`, `stack`, `box`, `button` | Vertical | Horizontal |

`align` accepts:

- `"start"`, the default
- `"center"`
- `"end"`

`justify` accepts:

- `"start"`, the default
- `"center"`
- `"end"`


Text, heading, button, and direct text items without an explicit `width` use an intrinsic width. An
auto-width container child occupies the available line width. 
A spacer adds gap of `size` along its parent's main axis
## Grammar

This grammar uses `::=` for definitions, `|` for alternatives, `*` for zero
or more, `+` for one or more, and `?` for an optional element.

```ebnf
document        ::= node
node            ::= ident "{" item* "}"
item            ::= property | text-child | node
property        ::= ident ":" literal
text-child      ::= string-literal

literal         ::= integer-lit | float-lit | length-lit | color-lit
                  | string-literal | bool-lit
integer-lit     ::= "-"? digit+
float-lit       ::= "-"? digit+ "." digit+
length-lit      ::= (integer-lit | float-lit) "px"
color-lit       ::= "#" hex-digit hex-digit hex-digit
                    (hex-digit hex-digit hex-digit)?
string-literal  ::= "\"" string-char* "\""
bool-lit        ::= "true" | "false"
ident           ::= (letter | "_") (letter | digit | "-" | "_")*
```

## Deferred to v2

State, bindings, assignment, event handlers, control flow, lists, `null`,
interpolation, general expressions, function calls, image nodes, non-`px`
lengths, and alpha colors are deliberately not valid v1 Yuloh.
