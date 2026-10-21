# CristalCube - Source encoding unification (CP949 / UTF-8 no BOM -> UTF-8 with BOM)
#
# Usage (PowerShell, run from anywhere):
#   1) Dry run (no file is changed, only prints what would happen):
#        powershell -ExecutionPolicy Bypass -File .\Convert-SourceToUtf8Bom.ps1
#   2) Apply (backs up every touched file first):
#        powershell -ExecutionPolicy Bypass -File .\Convert-SourceToUtf8Bom.ps1 -Apply
#
# Rules
#   - Already UTF-8 with BOM      -> skipped
#   - Pure ASCII                  -> skipped (no need; VS will save as UTF-8 BOM later via .editorconfig)
#   - Valid UTF-8 without BOM     -> BOM added, content unchanged
#   - Not valid UTF-8             -> decoded as CP949 and re-saved as UTF-8 with BOM
#   - Line endings are preserved as-is (LF files stay LF, CRLF files stay CRLF).
#   - Close Visual Studio / Rider before running -Apply.

param(
    [string]$Root   = "D:\MyWorks\PersonalPortfolio\CristalCube\Source",
    [string]$Backup = "D:\MyWorks\PersonalPortfolio\CristalCube\_EncodingBackup_20261021",
    [switch]$Apply
)

# PowerShell 7 (.NET Core) needs the code page provider for CP949; Windows PowerShell 5.1 has it built in.
try { [System.Text.Encoding]::RegisterProvider([System.Text.CodePagesEncodingProvider]::Instance) } catch {}

$cp949      = [System.Text.Encoding]::GetEncoding(949)
$utf8Strict = New-Object System.Text.UTF8Encoding($false, $true)   # throws on invalid bytes
$utf8Bom    = New-Object System.Text.UTF8Encoding($true)

$counts = @{ Skipped = 0; AddBom = 0; FromCp949 = 0 }

Get-ChildItem -Path $Root -Recurse -File -Include *.h, *.cpp, *.inl | ForEach-Object {
    $path  = $_.FullName
    $bytes = [System.IO.File]::ReadAllBytes($path)

    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    $isAscii = -not ($bytes | Where-Object { $_ -gt 127 } | Select-Object -First 1)

    if ($hasBom -or $isAscii) { $counts.Skipped++; return }

    try {
        $text = $utf8Strict.GetString($bytes)
        $kind = "UTF-8 (no BOM) -> add BOM"
        $counts.AddBom++
    }
    catch {
        $text = $cp949.GetString($bytes)
        $kind = "CP949 -> UTF-8 BOM"
        $counts.FromCp949++
    }

    $rel = $path.Substring($Root.Length).TrimStart('\')
    Write-Host ("[{0}] {1}" -f $kind, $rel)

    if ($Apply) {
        $dst = Join-Path $Backup $rel
        New-Item -ItemType Directory -Force -Path (Split-Path $dst) | Out-Null
        Copy-Item -LiteralPath $path -Destination $dst -Force
        [System.IO.File]::WriteAllText($path, $text, $utf8Bom)
    }
}

Write-Host ""
Write-Host ("Skipped (BOM/ASCII): {0} | Add BOM: {1} | CP949 converted: {2}" -f $counts.Skipped, $counts.AddBom, $counts.FromCp949)
if (-not $Apply) { Write-Host "Dry run only. Re-run with -Apply to convert (backup: $Backup)." }