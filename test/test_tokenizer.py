from textwrap import dedent

from rtf_tokenize import RtfTokenizer


TEST_RTF = dedent(
    r'''
    {\rtf1\ansi
    {\*\cxs TEFT}
    escaped newline: line\
    break
    \test1 ing\test2;
    }
    '''
).lstrip()

TEST_RTF_TOKENS = (
    '{', r'\rtf1', r'\ansi',
    '{', r'\*', r'\cxs', 'TEFT', '}',
    'escaped newline: line', '\\\n',
    'break',
    r'\test1', 'ing', r'\test2', ';',
    '}',
)

TEST_RTF_LOCATIONS = [
    tuple(map(int, loc.split(':')))
    for loc in '''
    0:0 0:1 0:6
    1:0 1:1 1:3 1:8 1:12
    2:0 2:21
    3:0
    4:0 4:7 4:10 4:16
    5:0
    '''.split()
]


def test_tokenizer_next_token():
    tokenizer = RtfTokenizer(TEST_RTF)
    for n, (expected_token, expected_loc) in enumerate(zip(TEST_RTF_TOKENS, TEST_RTF_LOCATIONS)):
        token = tokenizer.next_token()
        loc = (tokenizer.lnum, tokenizer.cnum)
        msg = 'token %u at %u:%u' % (n, loc[0], loc[1])
        assert token == expected_token, msg
        assert loc == expected_loc, msg
    msg = 'token %u at end' % (n + 1)
    expected_loc = (expected_loc[0] + 1, 0)
    assert tokenizer.next_token() is None, msg
    assert (tokenizer.lnum, tokenizer.cnum) == expected_loc, msg


def test_tokenizer_rewind_token():
    tokenizer = RtfTokenizer(TEST_RTF)
    # Read first 2 tokens.
    assert tokenizer.next_token() == TEST_RTF_TOKENS[0]
    assert (tokenizer.lnum, tokenizer.cnum) == TEST_RTF_LOCATIONS[0]
    assert tokenizer.next_token() == TEST_RTF_TOKENS[1]
    assert (tokenizer.lnum, tokenizer.cnum) == TEST_RTF_LOCATIONS[1]
    # Rewind 2 unrelated tokens.
    tokenizer.rewind_token('re')
    tokenizer.rewind_token('wind')
    # Check next 2 tokens are rewound one.
    assert tokenizer.next_token() == r'wind'
    assert (tokenizer.lnum, tokenizer.cnum) == TEST_RTF_LOCATIONS[1]
    assert tokenizer.next_token() == r're'
    assert (tokenizer.lnum, tokenizer.cnum) == TEST_RTF_LOCATIONS[1]
    # And that we continue where we left.
    assert tokenizer.next_token() == TEST_RTF_TOKENS[2]
    assert (tokenizer.lnum, tokenizer.cnum) == TEST_RTF_LOCATIONS[2]
