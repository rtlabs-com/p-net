.. _writing-documentation:

Writing documentation
=====================
Documentation can be written in Markdown or reStructuredText (reST).

Use restructured text when more advanced featured are required, for example
inserting a table of contents.

Add new documentation files to ``doc/index.rst``.


Markdown
--------
Use ``.md`` file name extension.
Parsing is done with the Commonmark specification,
see https://commonmark.org/


Restructured text
-----------------
Use ``.rst`` file name extension.
For details on the syntax, see
https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html


Installing documentation toolchain
----------------------------------

Install Sphinx by running this::

    $ sudo apt install python3-pip
    $ pip3 install -r doc/doc_requirements.txt

Build documentation::

    $ cmake --build build/ --target sphinx-html
    $ cmake --build build/ --target sphinx-pdf

Run checks::

    $ cmake --build build/ --target sphinx-spelling
    $ cmake --build build/ --target sphinx-linkcheck
    $ cmake --build build/ --target codespell
