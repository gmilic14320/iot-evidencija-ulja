from pathlib import Path
import tempfile
import PyInstaller.__main__

folder_koda = Path(__file__).resolve().parent
projekat = folder_koda.parents[1]
# Pravi aplikaciju, a privremene fajlove uklanja po završetku.
with tempfile.TemporaryDirectory() as privremeni_folder:
    PyInstaller.__main__.run([
        "--noconfirm", "--windowed", "--name", "Podesi tank",
        "--distpath", str(projekat), "--workpath", str(Path(privremeni_folder) / "build"),
        "--specpath", privremeni_folder, "--paths", str(folder_koda),
        str(folder_koda / "podesi_tank.pyw"),
    ])
