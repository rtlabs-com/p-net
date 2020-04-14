Installing documentation toolchain
==================================

Install Sphinx::

    doc$ sudo apt install python3-pip
    doc$ pip3 install -r doc_requirements.txt

Build documentation::

    doc$ make html
    doc$ make pdf

Run checks::

    doc$ make spelling
    doc$ make linkcheck
