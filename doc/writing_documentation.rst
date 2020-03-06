Installing documentation toolchain
==================================

Install Sphinx::

    sudo apt install python3-pip
    pip3 install -U sphinx
    pip3 install -U recommonmark
    pip3 install -U sphinx-rtd-theme
    pip3 install -U breathe
    pip3 install -U sphinxcontrib-spelling

Build documentation::

    doc$ make html

Run checks::

    doc$ make spelling
    doc$ make linkcheck
