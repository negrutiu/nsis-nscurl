# Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2020/02/15

# ======================================================

$z7 = "${env:ProgramFiles}\7-zip\7z.exe"
if (!(ls $z7 -ErrorAction:Ignore)) {
	Write-Host -ForegroundColor white -BackgroundColor red "ERROR: Missing ${z7}"; Pause; Exit
}

# ======================================================

$dn = "libcurl-devel"
$fn = "libcurl-devel-negrutiu.7z"
$uri = "https://github.com/negrutiu/libcurl-devel/releases/latest/download/${fn}"
Push-Location $PSScriptRoot

# ======================================================

""
Write-Host -ForegroundColor yellow -BackgroundColor darkcyan " Downloading... "
""

if (ls $fn -ErrorAction:Ignore) {
	$headers = @{ 'If-Modified-Since' = (ls $fn).LastWriteTimeUtc.ToString("ddd, dd MMM yyyy HH:mm:ss", [CultureInfo]::CreateSpecificCulture('en-US')) + " GMT" }
}

try {
	Invoke-WebRequest -Uri $uri -OutFile $fn -Headers $headers -ErrorAction:Stop
	$StatusCode = 200
	$StatusDescription = "OK"
} catch {
	$StatusCode = $_.Exception.Response.StatusCode.value__
	$StatusDescription = $_.Exception.Response.StatusDescription
}

if (($StatusCode -ge 200) -and ($StatusCode -lt 300)) {
    Write-Host -ForegroundColor green "${StatusCode} ${StatusDescription}"
    # Remove existing files. Extract again
	Remove-Item -Path $dn -Force -Recurse -ErrorAction:Ignore
} elseif (($StatusCode -ge 300) -and ($StatusCode -lt 400)) {
    Write-Host -ForegroundColor yellow "${StatusCode} ${StatusDescription}"
} else {
    Write-Host -ForegroundColor red "${StatusCode} ${StatusDescription}"
}

# ======================================================

""
Write-Host -ForegroundColor yellow -BackgroundColor darkcyan " Extracting... "
""

if (!(ls $dn -ErrorAction:Ignore)) {
	Start-Process $z7 -ArgumentList "x", "${fn}", "-o${dn}", "*", "-r" -NoNewWindow -PassThru -Wait -ErrorAction:Ignore
} else {
    "Already did..."
}

Remove-Variable headers -ErrorAction:Ignore
#Remove-Variable response -ErrorAction:Ignore
Pop-Location

""
Pause