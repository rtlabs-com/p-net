# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import time

# Workaround for issue https://github.com/sphinx-contrib/googleanalytics/issues/2
# Note that a warning still will be issued "unsupported object from its setup() function"
# Remove this workaround when the issue has been resolved upstream
import sphinx.application
import sphinx.errors
sphinx.application.ExtensionError = sphinx.errors.ExtensionError


# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'p-net'
copyright = '2020, rt-labs'
author = 'rt-labs'


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "breathe",
    "recommonmark",
    "rst2pdf.pdfbuilder",
    "sphinx_rtd_theme",
    "sphinxcontrib.spelling",
    "sphinxcontrib.googleanalytics",
]

needs_sphinx = "1.8"

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

spelling_word_list_filename = "spelling_wordlist.txt"

googleanalytics_id = "UA-4171737-2"

googleanalytics_enabled = True

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ['static']

html_last_updated_fmt = '%Y-%m-%d %H:%M'
breathe_projects = { project: "../build/xml/" }
breathe_default_project = project

html_css_files = [
    '../../css/custom_rtd.css',  # Requested by web developer
    'css/fix_table_width.css',
    'css/change_header_size.css'
    ]


# -- Options for PDF output -------------------------------------------------

pdf_filename = "{}_{}".format(project.replace("-", ""), time.strftime("%Y-%m-%d"))
pdf_title = "{} Profinet device stack".format(project)
pdf_documents = [('index', pdf_filename, pdf_title, author),]

pdf_break_level = 2
