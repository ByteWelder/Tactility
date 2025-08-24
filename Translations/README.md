# Translations

## Requirements

- Install [LibreOffice Calc](https://libreoffice.org/)
- Ensure you have Python 3 installed (Python 2 is not supported)

## Steps

To add new translations or edit existing ones, please follow these steps:

1. Edit `Translations.ods` (see chapter below)
2. In LibreOffice Calc, select the relevant tab that you want to export
3. Click on `File` -> `Save a copy...` and save the file as `[tabname].csv` (without the "[]")
4. Repeat step 2 and 3 for all tabs that you updated
5. Run `python generate-all.py`

## Notes

- Do not commit the CSV files
- When editing the ODS file, make sure you don't paste in formatted data (use CTRL+Shift+V instead of CTRL+V)
- ODS export settings:
    - Field delimiter: `,`
    - String delimiter: `"`
    - Encoding: `UTF-8`