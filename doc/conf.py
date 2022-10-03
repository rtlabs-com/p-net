# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import pathlib
import re
import sys
import time

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))

pathobj_docs_dir = pathlib.Path(__file__).parent.absolute()
pathobj_rootdir = pathobj_docs_dir.parent.absolute()

# -- Project information -----------------------------------------------------

try:
    cmakelists_contents = pathobj_rootdir.joinpath("CMakeLists.txt").read_text()
    versiontext_match = re.search(r"PROFINET VERSION ([\d.]*)", cmakelists_contents)
    version = versiontext_match.group(1)
except:
    version = "unknown version"

project = 'p-net'
copyright = '2023, RT-Labs'
author = 'RT-Labs'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "breathe",
    "myst_parser",
    "rst2pdf.pdfbuilder",
    "sphinx_rtd_theme",
    "sphinxcontrib.spelling",
]

needs_sphinx = "1.8"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

spelling_word_list_filename = "spelling_wordlist.txt"

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["static"]

html_theme_options = {
    "analytics_id": "G-378E9EVTG5",
    "display_version": True,
    "navigation_depth": 2,
}

html_last_updated_fmt = "%Y-%m-%d %H:%M"
breathe_projects = {project: "../build/doc/xml/"}
breathe_default_project = project

html_css_files = [
    "../../css/custom_rtd.css",  # Requested by web developer
    "css/fix_table_width.css",
    "css/change_header_size.css",
]


# -- Options for PDF output -------------------------------------------------

pdf_filename = "{}_{}".format(project.replace("-", ""), time.strftime("%Y-%m-%d"))
pdf_title = "{} Profinet device stack".format(project)
pdf_documents = [
    ("index", pdf_filename, pdf_title, author),
]

pdf_break_level = 2
