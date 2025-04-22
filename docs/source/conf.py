# Configuration file for the Sphinx documentation builder.
#
# For the full list of built-in configuration values, see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

import os
import sys
import tomllib

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
