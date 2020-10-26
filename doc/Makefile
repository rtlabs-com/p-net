# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

htmlzipfile := pnet_docs_$(shell git describe --dirty --always --tags).zip

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
# Note that we remove the build status badge image before PDF creation.
%: Makefile

	@echo  "Cleaning ..."
	@rm  -rf _copied/
	@rm  -rf _build/
	@echo  "Copying markdown files to _copied subdir"
	@mkdir -p _copied
	@cp ../README.md _copied/
	@cp ../LICENSE.md _copied/
	@cp ../CHANGELOG.md _copied/
	@(test $@ != pdf) || sed -i '/Build Status/d' _copied/README.md
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	@(test $@ != html) || cd _build; echo  "Zipping ..."; zip -r $(htmlzipfile) html/ > /dev/null
