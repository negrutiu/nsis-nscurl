#!/usr/bin/env pwsh

<#
.SYNOPSIS
    Update "LIBCURL_VERSION" and "LIBCURL_TIMESTAMP" in "include/curl/curlver.h" with correct values.

.Parameter version 
    Optional curl version (i.e. "8.8.0"). If not specified, it's read from "vcpkg.json".

.Parameter tag
    Optional curl git tag (i.e. "curl-8_8_0"). If not specified, it's automatically constructed from $version.

.Parameter sources 
    Optional curl sources root directory. If not specified, the current directory (Get-Location) is used.
#>

Param(
    [Parameter(Mandatory = $false)] [string] $version = "",
    [Parameter(Mandatory = $false)] [string] $tag = "",
    [Parameter(Mandatory = $false)] [string] $sources = ""
)

if (-not $version) {
    $vcpkgJson = Join-Path (Split-Path -Parent $PSScriptRoot) 'vcpkg.json'
   	$json = Get-Content -Path $vcpkgJson -ErrorAction SilentlyContinue | Out-String | ConvertFrom-Json
    $version = $json.version
}
if (-not $tag) {
    $tag = 'curl-' + $version.replace('.', '_')
}
if (-not $sources) {
    $sources = Get-Location
}

"-- version: ${version}"
"-- tag:     ${tag}"
"-- sources: ${sources}"

# Look for curlver.h
$curlverHeader = Join-Path $sources 'include/curl/curlver.h'
$curlverContent = Get-Content $curlverHeader | Out-String
$curlverDirty = $false

if ($curlverContent | Select-String -Pattern "`"${version}-DEV`"" -SimpleMatch)
{
    $curlverContent = $curlverContent.replace("`"${version}-DEV`"", "`"${version}`"")
    $curlverDirty = $true
    "replaced `"${version}-DEV`" with `"${version}`""
}

if ($curlverContent | Select-String -Pattern "`"[unreleased]`"" -SimpleMatch)
{
    # Read the release date from GitHub
    $url = "https://api.github.com/repos/curl/curl/releases/tags/${tag}"
    "GET ${url}"
    $response = Invoke-RestMethod -Method GET -Uri $url
    # $response

    $releaseDate = ([datetime]$response.published_at).ToString("yyyy-MM-dd")

    $curlverContent = $curlverContent.replace("`"[unreleased]`"", "`"${releaseDate}`"")
    $curlverDirty = $true
    "replaced `"[unreleased]`" with `"${releaseDate}`""
}

if ($curlverDirty)
{
    $curlverContent | Out-File -FilePath $curlverHeader -Encoding utf8
    ">>> updated ${curlverHeader}"
}
else
{
    ">>> no need to update ${curlverHeader}"
}
