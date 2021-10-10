#!/usr/bin/env python3

from setuptools import Extension, setup


setup(
    ext_modules=[
        Extension('rtf_tokenize',
                  sources=['rtf_tokenize.c']),
    ],
)
