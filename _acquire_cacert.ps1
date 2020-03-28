$fn = "cacert.pem"
$uri = "https://curl.haxx.se/ca/cacert.pem"
Push-Location $PSScriptRoot

if (ls $fn -ErrorAction:Ignore) {
	$headers = @{ 'If-Modified-Since' = (ls $fn).LastWriteTimeUtc.ToString("ddd, dd MMM yyyy hh:mm:ss", [CultureInfo]::CreateSpecificCulture('en-US')) + " GMT" }
}

$response = try {
    # Returns WebResponseObject. Convert to HttpWebResponse
	[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
	(Invoke-WebRequest -Uri $uri -OutFile $fn -Headers $headers -PassThru).BaseResponse
} catch {
    # Returns HttpWebResponse
    $_.Exception.Response
}

if (($response.StatusCode.value__ -ge 200) -and ($response.StatusCode.value__ -lt 300)) {
    Write-Host -ForegroundColor green $response.StatusCode.value__ $response.StatusDescription
} elseif (($response.StatusCode.value__ -ge 300) -and ($response.StatusCode.value__ -lt 400)) {
    Write-Host -ForegroundColor yellow $response.StatusCode.value__ $response.StatusDescription
} else {
    Write-Host -ForegroundColor red $response.StatusCode.value__ $response.StatusDescription
}
$response.Headers.ToString()

Remove-Variable headers -ErrorAction:Ignore
#Remove-Variable response -ErrorAction:Ignore
Pop-Location

if (!([Environment]::GetCommandLineArgs() -like '-noni*')) {
	Pause
}
