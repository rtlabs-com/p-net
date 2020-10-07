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

Install Sphinx by running this in the ``doc`` subdirectory::

    doc$ sudo apt install python3-pip
    doc$ pip3 install -r doc_requirements.txt

Build documentation::

    doc$ make html
    doc$ make pdf

Run checks::

    doc$ make spelling
    doc$ make linkcheck
