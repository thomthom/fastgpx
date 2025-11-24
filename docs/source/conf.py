# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import logging
import os
import re
import sys
from typing import TYPE_CHECKING

import sphinx
import tomllib

if TYPE_CHECKING:
    import sphinx.application


PROJECT_ROOT = os.path.abspath('../../')

sys.path.insert(0, os.path.join(PROJECT_ROOT, 'src'))

# Path: docs/source/_ext
# ext_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "_ext"))
# sys.path.insert(0, ext_path)

# -- Project information -----------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#project-information

# Read the version from pyproject.toml
with open(os.path.join(PROJECT_ROOT, 'pyproject.toml'), 'rb') as f:
    pyproject = tomllib.load(f)
    pyproject_name = pyproject['project']['name']
    pyproject_version = pyproject['project']['version']

assert pyproject['project']['name'] is not None
assert pyproject['project']['version'] is not None

project = pyproject['project']['name']
author = 'Thomas Thomassen'
copyright = f'2024-2025, {author}'
release = pyproject['project']['version']

# -- General configuration ---------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#general-configuration

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.autosummary',
    'sphinx.ext.napoleon',  # For NumPy and Google style docstrings
    'sphinx_autodoc_typehints',
    'sphinx_rtd_theme',
]

templates_path = ['_templates']
# exclude_patterns = []

autosummary_generate = True

# autodoc_typehints = "description"
# autodoc_typehints = "both"
autodoc_typehints = "signature"
autodoc_typehints_description_target = "all"

# https://github.com/tox-dev/sphinx-autodoc-typehints?tab=readme-ov-file#options
typehints_fully_qualified = False
always_document_param_types = True
always_use_bars_union = True


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']


logger = logging.getLogger(__name__)

# https://www.sphinx-doc.org/en/master/usage/extensions/autodoc.html#event-autodoc-process-signature
# Emitted when autodoc has formatted a signature for an object. The event handler can return a new
# tuple (signature, return_annotation) to change what Sphinx puts into the output.


def autodoc_process_signature(app, what, name, obj, options, signature, return_annotation):
    # https://github.com/wjakob/nanobind/discussions/707#discussioncomment-11708375
    logger.debug('')
    logger.debug('autodoc_process_signature')
    logger.debug(f"What: \033[35m{what}\033[0m, Name: \033[36m{name}\033[0m")
    logger.debug(f"Obj: {obj}")
    logger.debug(f"signature : \033[32m{signature}\033[0m")
    logger.debug(f"Doc:\n\033[33m{obj.__doc__}\033[0m")
    logger.debug('')
    # Fix signatures to use "fastgpx." instead of "fastgpx.fastgpx."
    # https://github.com/sphinx-doc/sphinx/issues/10351
    # Only the first `@overload` is given to this callback.
    if signature:
        signature = signature.replace("fastgpx.fastgpx.", "fastgpx.")
    if return_annotation:
        return_annotation = return_annotation.replace(
            "fastgpx.fastgpx.", "fastgpx.")
    return signature, return_annotation


def autodoc_before_process_signature(app, obj, bound_method):
    logger.debug('')
    logger.debug('autodoc_before_process_signature')
    logger.debug(f"Obj: {obj}")
    logger.debug(f"Doc:\n{obj.__doc__}")
    logger.debug('')


def autodoc_process_docstring(app, what, name, obj, options, lines):
    # https://www.sphinx-doc.org/en/master/usage/extensions/autodoc.html#event-autodoc-process-docstring
    # Emitted when autodoc has read and processed a docstring. lines is a list
    # of strings – the lines of the processed docstring – that the event handler
    # can modify in place to change what Sphinx puts into the output.
    logger.debug('')
    logger.debug('autodoc_process_docstring')
    logger.debug(f"What: \033[35m{what}\033[0m, Name: \033[36m{name}\033[0m")
    logger.debug(f"Obj: {obj}")
    logger.debug(f"Lines: \n\033[33m{lines}\033[0m")
    logger.debug('')
    if what == 'property' and lines:
        logger.debug('\033[35mFixing property docstring...\033[0m')
        # nanobind + sphinx + autodoc seems to generate a docstring for
        # properties that includes the signature line. The PrettyPropertyDocumenter
        # also tries to parse this line to extract the return type.
        # Here we want to remove that signature line from the docstring to avoid
        # duplication.
        #
        # Example sig_line:
        #   (self) -> fastgpx.fastgpx.LatLong | None
        #
        # for line in lines:
        #     match = re.match(r'^\(.*\)\s*->\s*(.*)$', line)
        # Delete the first line if it matches the signature pattern
        match = re.match(r'^\(.*\)\s*->\s*(.*)$', lines[0])
        if match:
            lines.pop(0)


def monkey_patch_property_documenter(app: sphinx.application.Sphinx):
    from sphinx.ext.autodoc import PropertyDocumenter

    class MonkeyPatchPropertyDocumenter(PropertyDocumenter):
        # before PropertyDocumenter
        priority = PropertyDocumenter.priority + 1

        def add_directive_header(self, sig: str) -> None:
            super().add_directive_header(sig)
            sourcename = self.get_sourcename()

            logger.debug(
                f"\033[35mMonkeyPatchPropertyDocumenter adding directive header for {self.object}\033[0m")

            lines = self.get_doc()
            if not lines:
                return

            sig_line = lines[0][0]

            # Example sig_line:
            #   (self) -> fastgpx.fastgpx.LatLong | None
            #
            # We want to extract the return type:
            match = re.match(r'^\(.*\)\s*->\s*(.*)$', sig_line)
            if not match:
                return

            types = match.group(1)
            if types:
                types = types.replace("fastgpx.fastgpx.", "fastgpx.")

            objrepr = types
            self.add_line('   :type: ' + objrepr, sourcename)

    app.add_autodocumenter(MonkeyPatchPropertyDocumenter, override=True)


def monkey_patch_sphinx_for_nanobind():
    # Patch based on this discussion:
    # https://github.com/wjakob/nanobind/discussions/707#discussioncomment-13540168
    #
    # A more complete monkey patch for nanobind objects in Sphinx:
    # https://github.com/wjakob/nanobind/discussions/707#discussioncomment-15036945
    import inspect
    from sphinx.util import inspect as sphinx_inspect

    # Sphinx inspects all objects in the module and tries to resolve their type
    # (attribute, function, class, module, etc.) by using its own functions in
    # `sphinx.util.inspect`. These functions misidentify certain nanobind
    # objects. We monkey patch those functions here.
    def mpatch_ismethod(object):
        if hasattr(object, '__name__') and type(object).__name__ == "nb_method":
            return True
        return inspect.ismethod(object)

    sphinx_inspect_isclassmethod = sphinx_inspect.isclassmethod

    def mpatch_isclassmethod(object, cls=None, name=None):
        if hasattr(object, '__name__') and type(object).__name__ == "nb_method":
            return False
        return sphinx_inspect_isclassmethod(object, cls, name)

    sphinx_inspect_isfunction = sphinx_inspect.isfunction

    def mpatch_isfunction(object):
        if hasattr(object, '__name__') and type(object).__name__ == "nb_func":
            return True
        return sphinx_inspect_isfunction(object)

    sphinx_inspect.ismethod = mpatch_ismethod
    sphinx_inspect.isclassmethod = mpatch_isclassmethod
    sphinx_inspect.isfunction = mpatch_isfunction


def setup(app: sphinx.application.Sphinx):
    if app.verbosity >= 1:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)
    handler = logging.StreamHandler(sys.stdout)
    handler.setLevel(logging.DEBUG)
    logger.addHandler(handler)

    app.connect('autodoc-process-docstring', autodoc_process_docstring)
    app.connect('autodoc-process-signature', autodoc_process_signature)
    # app.connect('autodoc-before-process-signature', autodoc_before_process_signature)

    monkey_patch_property_documenter(app)
    monkey_patch_sphinx_for_nanobind()
