import hashlib
import os
import zipfile

NEW_PROJECT_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "TactilitySDK", "NewProject")
ARCHIVE_NAME = "new-project.zip"

def create_new_project_zip(out_path: str):
    """Zips the project templates that tt.py generates a new app from. Returns the entry of index.json."""
    archive_path = os.path.join(out_path, ARCHIVE_NAME)
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for directory, directory_names, file_names in os.walk(NEW_PROJECT_PATH):
            directory_names.sort()
            for file_name in sorted(file_names):
                file_path = os.path.join(directory, file_name)
                archive.write(file_path, os.path.relpath(file_path, NEW_PROJECT_PATH))
    with open(archive_path, "rb") as archive:
        sha256 = hashlib.sha256(archive.read()).hexdigest()
    return {"file": ARCHIVE_NAME, "sha256": sha256}
