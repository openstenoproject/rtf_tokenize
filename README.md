# RTF Tokenize

A simple RTF tokenizer.

API:

``` python
# Init:
from rtf_tokenize import RtfTokenizer
tokenizer = RtfTokenizer(rtf_string)
# Get next token (return `None` on end of string):
token = tokenizer.next_token()
# Rewind a token (next call to `next_token` will return it):
tokenizer.rewind_token(token)
# Current location (last tokenized, irrespective of rewound tokens):
position = tokenizer.lnum, tokenizer.cnum
```


## Release history

### 1.0.1

* add copy of LICENSE.txt to distribution

### 1.0.0

* first public release
