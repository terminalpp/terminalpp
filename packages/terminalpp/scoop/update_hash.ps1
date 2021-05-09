Import-Module Microsoft.PowerShell.Utility
$hash = Get-FileHash terminalpp-portable.exe -Algorithm SHA256
$scoop = Get-Content terminalpp-scoop.json
$updated = $scoop -Replace 'fill_in_the_hash_of_terminalpp_portable_exe', $hash.hash
$updated | Set-Content terminalpp-scoop.json
