# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html


import os
import pathlib
import re
import sys
import time

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.

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
    "recommonmark",
    "sphinx_copybutton",
    "sphinx_jinja",
    "sphinx.ext.autosectionlabel",
    "sphinx.ext.graphviz",
    "sphinxcontrib.kroki",
    "sphinxcontrib.programoutput",
    "sphinxcontrib.spelling",
]

needs_sphinx = "1.8"

# Add any paths that contain templates here, relative to this directory.
templates_path = ["../../_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

spelling_word_list_filename = "spelling_wordlist.txt"

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

breathe_projects = {project: "../build/doc/xml/"}
breathe_default_project = project


# -- Options for HTML output -------------------------------------------------

html_context = {
   "default_mode": "light"
}

html_theme = "sphinx_book_theme"
html_theme_options = {
    "show_nav_level": 3,
    "home_page_in_toc": True,
    "use_repository_button": True,
    "use_fullscreen_button": False,
    "navbar_end": ["navbar-icon-links"],
    "use_download_button": False,
    "repository_url": "https://github.com/rtlabs-com/p-net",
}

html_last_updated_fmt = "%Y-%m-%d %H:%M"
html_static_path = ["static"]
html_logo = "static/i/p-net.svg"
html_show_sourcelink = False

if os.getenv("USE_EXTERNAL_CSS") is not None:
    html_css_files = [
        "https://rt-labs.com/content/themes/rtlabs2020/assets/css/style.css",
        "https://rt-labs.com/content/themes/rtlabs2020/assets/css/rt_custom_sphinx.css",
    ]


# -- Options for PDF output -------------------------------------------------

pdf_filename = "{}_{}".format(project.replace("-", ""), time.strftime("%Y-%m-%d"))
pdf_title = "{} Profinet device stack".format(project)
pdf_documents = [
    ("index", pdf_filename, pdf_title, author),
]

pdf_break_level = 2
