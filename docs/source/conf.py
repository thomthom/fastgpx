# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import re
import os
import sys
import tomllib
import logging as std_logging
from sphinx.util import logging

PROJECT_ROOT = os.path.abspath('../../')

sys.path.insert(0, os.path.join(PROJECT_ROOT, 'src'))

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
    'sphinx_rtd_theme',
]

templates_path = ['_templates']
# exclude_patterns = []

autosummary_generate = True


# -- Options for HTML output -------------------------------------------------
# https://www.sphinx-doc.org/en/master/usage/configuration.html#options-for-html-output

html_theme = 'sphinx_rtd_theme'
html_static_path = ['_static']


# -- Adjust formatting for overloaded methods --------------------------------

logger = logging.getLogger(__name__)


def format_overloaded_docstring(app, what, name, obj, options, lines):
    logger.info(f"[fastgpx-doc] Processing docstring for {name}")
    if not lines:
        return

    logger.info(f"[fastgpx-doc] First line: {lines[0]}")
    if lines[0].strip().startswith("Overloaded function"):
        logger.info(f"[fastgpx-doc] Detected overloaded function in {name}")

        new_lines = [".. rubric:: Overloads", ""]

        for line in lines[1:]:
            match = re.match(r"^\d+\.\s+(.*)", line.strip())
            if match:
                new_lines.extend([
                    ".. code-block:: python",
                    "",
                    f"    {match.group(1)}",
                    ""
                ])

        lines.clear()
        lines.extend(new_lines)


def setup(app):
    if app.verbosity >= 1:
        logger.setLevel(std_logging.INFO)
    else:
        logger.setLevel(std_logging.WARNING)

    logger.info("[fastgpx-doc] Setting up overloaded docstring formatting")
    app.connect("autodoc-process-docstring", format_overloaded_docstring)
