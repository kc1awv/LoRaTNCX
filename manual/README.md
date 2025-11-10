# LoRaTNCX User Manual

This directory contains the user manual for LoRaTNCX, organized as separate Markdown files for easier maintenance and contribution.

## Structure

- `user_manual.md` - Main manual index with table of contents
- `sections/` - Individual section files (01-introduction.md through 17-appendices.md)

## Building the Manual

### Single PDF

To create a single PDF from all sections:

```bash
# Install Pandoc if not already installed
pip install pandoc

# Build the PDF
pandoc user_manual.md sections/*.md -o LoRaTNCX_User_Manual.pdf
```

### HTML Website

To create a static HTML website:

```bash
# Using MkDocs (recommended for documentation sites)
pip install mkdocs mkdocs-material

# Create mkdocs.yml configuration
# Then build
mkdocs build
```

### GitHub Pages

The manual is designed to work well with GitHub's Markdown rendering. Each section links back to the main manual and between sections.

## Contributing

1. Edit the appropriate section file in `sections/`
2. Ensure navigation links remain valid
3. Test that the manual builds correctly
4. Submit a pull request

## File Naming Convention

- `01-introduction.md` - Introduction section
- `02-getting-started.md` - Getting started guide
- etc.

This numbering ensures proper ordering when combining files.