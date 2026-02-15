#Requires -Version 5
param (
	[Parameter(Mandatory)][ValidateSet('SOURCEGEN', 'DISTRIBUTE')][string]$Mode = 'SOURCEGEN',
	[string]$Version,
	[string]$Path,
	[string]$Payload
)

$ErrorActionPreference = "Stop"
$Folder = $PSScriptRoot | Split-Path -Leaf
$SourceExt = @('.asm', '.c', '.cc', '.cpp', '.cxx', '.h', '.hpp', '.hxx', '.inc', '.inl', '.ixx')

function Normalize-Path {
	param([string]$in)
	$out = $in -replace '\\', '/'
	while ($out.Contains('//')) { $out = $out -replace '//', '/' }
	return $out
}

function Resolve-Files {
	param(
		[string]$parent = $PSScriptRoot,
		[string[]]$range = @('include', 'src')
	)
	$generated = [System.Collections.ArrayList]::new()
	Push-Location $PSScriptRoot
	try {
		foreach ($directory in $range) {
			Get-ChildItem "$parent/$directory" -Recurse -File -ErrorAction SilentlyContinue |
				Where-Object { $_.Extension -in $SourceExt -and $_.Name -notmatch 'Plugin\.h|Version\.h|PCH\.h' } |
				Resolve-Path -Relative | ForEach-Object { $generated.Add("`n`t`"$($_.ToString().Substring(2) -replace '\\','/')`"") | Out-Null }
		}
	} finally { Pop-Location }
	return $generated
}

Write-Host "`n`t<$Folder> [$Mode]"

if ($Mode -eq 'SOURCEGEN') {
	Remove-Item "$Path/sourcelist.cmake" -Force -ErrorAction SilentlyContinue
	$generated = 'set(SOURCES'
	$generated += $PSScriptRoot | Resolve-Files
	if ($Path) { $generated += $Path | Resolve-Files }
	$generated += "`n)"
	[IO.File]::WriteAllText("$Path/sourcelist.cmake", $generated)
}

$RuleVarTbl = @{ config = ($Path -split '/')[-1].ToLower(); cmake_output = Normalize-Path ($Path + '/'); dist = Normalize-Path "$PSScriptRoot/dist/"; project_name = $Payload }
$RuleCmds = @()

function Resolve-RuleVar {
	param([string]$Path)
	$Resolved = $Path
	foreach ($unset in [regex]::Matches($Path, '\{.*?\}').Value) {
		$inner = $unset.Trim('{', '}')
		if ($inner.StartsWith('env:')) { $Resolved = $Resolved -replace [regex]::Escape($unset), [System.Environment]::GetEnvironmentVariable($inner.TrimStart('env:')) }
		else { $Resolved = $Resolved -replace [regex]::Escape($unset), (if ($script:RuleVarTbl.Contains($inner)) { $script:RuleVarTbl[$inner] } else { $inner }) }
	}
	return Normalize-Path $Resolved.Trim('{', '}')
}

if ($Mode -eq 'DISTRIBUTE') {
	$Path = Normalize-Path $Path
	$RuleVarTbl.cmake_output = $Path + '/'
	$RuleVarTbl.dist = Normalize-Path "$PSScriptRoot/dist/"
	# Copy build output to dist
	$script:RuleCmds += "New-Item '$($RuleVarTbl.dist)' -ItemType Directory -Force -ErrorAction:SilentlyContinue"
	$script:RuleCmds += "Copy-Item '$($RuleVarTbl.cmake_output)*' '$($RuleVarTbl.dist)' -Force -Recurse -ErrorAction:SilentlyContinue"
	# Copy to game if SFPath set
	$sfPath = [System.Environment]::GetEnvironmentVariable('SFPath')
	if ($sfPath) {
		$dest = Normalize-Path "$sfPath/Data/SFSE/Plugins/"
		$script:RuleCmds += "New-Item '$dest' -ItemType Directory -Force -ErrorAction:SilentlyContinue"
		$script:RuleCmds += "Copy-Item '$($RuleVarTbl.cmake_output)ResourceTracker.dll' '$dest' -Force -ErrorAction:SilentlyContinue"
	}
	$RuleCmds | Out-File "$($RuleVarTbl.dist)/deploy-$($RuleVarTbl.config).ps1" utf8
	& "$($RuleVarTbl.dist)/deploy-$($RuleVarTbl.config).ps1"
}
