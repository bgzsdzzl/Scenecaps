[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Version,

    [string]$OutputDir = "dist",

    [string]$RepoRoot = (Join-Path $PSScriptRoot "..")
)

$ErrorActionPreference = "Stop"
$Version = $Version -replace '^v', ''
$RepoRoot = (Resolve-Path $RepoRoot).Path
$OutputDir = Join-Path $RepoRoot $OutputDir

$uplugin = Get-Content (Join-Path $RepoRoot "Scenecaps.uplugin") -Raw | ConvertFrom-Json
if ($uplugin.VersionName -ne $Version) {
    throw "版本不匹配: tag '$Version' vs uplugin '$($uplugin.VersionName)'"
}

function Get-ChangelogSection {
    param([string]$Content, [string]$Ver, [string]$FallbackLabel)

    $patterns = @("## [$Ver]", "## [v$Ver]", $FallbackLabel)
    $lines = $Content -split "`r?`n"

    foreach ($pattern in $patterns) {
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if (-not $lines[$i].StartsWith($pattern)) { continue }

            $end = $lines.Count
            for ($j = $i + 1; $j -lt $lines.Count; $j++) {
                if ($lines[$j] -match '^## ' -or $lines[$j].Trim() -eq '---') {
                    $end = $j
                    break
                }
            }
            return ($lines[($i + 1)..($end - 1)] -join "`n").Trim()
        }
    }
    return ""
}

$changelog = Get-Content (Join-Path $RepoRoot "CHANGELOG.md") -Raw
$section = Get-ChangelogSection -Content $changelog -Ver $Version -FallbackLabel "## [未发布]"
$sectionSource = if ($section -and (Get-ChangelogSection -Content $changelog -Ver $Version -FallbackLabel "")) { "正式版本" } else { "未发布区（建议先将 CHANGELOG.md 中的 [未发布] 改为 [$Version]）" }
if (-not $section) { $section = "（CHANGELOG 中暂无该版本的记录）" }

$sourceItems = @("Source", "Config", "Localization", "Scenecaps.uplugin", "README.md", "LICENSE", "CHANGELOG.md")
$docsItems   = @("docs", "README.md", "CHANGELOG.md", "CONTRIBUTING.md", "CODE_OF_CONDUCT.md", "LICENSE")
$fullItems   = @($sourceItems + $docsItems | Select-Object -Unique)

function New-ReleasePackage {
    param([string]$Name, [string[]]$Items)

    $stage = Join-Path ([IO.Path]::GetTempPath()) "scenecaps-pkg-$Name-$(Get-Random)"
    New-Item -ItemType Directory -Path $stage -Force | Out-Null

    foreach ($item in $Items) {
        $src = Join-Path $RepoRoot $item
        if (-not (Test-Path $src)) {
            Write-Warning "跳过不存在的条目: $item"
            continue
        }
        Copy-Item $src (Join-Path $stage $item) -Recurse -Force
    }

    $zip = Join-Path $OutputDir "$Name.zip"
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
    Remove-Item $stage -Recurse -Force
    return $zip
}

New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

$packages = @(
    @{ Name = "Scenecaps-$Version-source"; Items = $sourceItems; Desc = "插件源码（Source/Config/Localization），需引擎编译" }
    @{ Name = "Scenecaps-$Version-docs";   Items = $docsItems;   Desc = "文档包（docs + 贡献指南 + 更新日志）" }
    @{ Name = "Scenecaps-$Version-full";   Items = $fullItems;   Desc = "源码 + 文档完整包" }
)

$built = foreach ($pkg in $packages) {
    $zip = New-ReleasePackage -Name $pkg.Name -Items $pkg.Items
    $hash = (Get-FileHash $zip -Algorithm SHA256).Hash
    [PSCustomObject]@{
        File    = (Split-Path $zip -Leaf)
        Desc    = $pkg.Desc
        SizeMB  = [math]::Round((Get-Item $zip).Length / 1MB, 2)
        Sha256  = $hash
    }
}

$notesPath = Join-Path $OutputDir "RELEASE_NOTES.md"
$lines = @(
    "# Scenecaps v$Version",
    "",
    "非破坏性场景引用集合资产插件，兼容 Lumen 距离场。",
    "",
    "## 变更记录",
    "",
    "> 来源：$sectionSource",
    "",
    $section,
    "",
    "## 下载",
    "",
    "| 包 | 内容 | 大小 | SHA256 |",
    "|---|---|---|---|"
)
foreach ($p in $built) {
    $lines += "| ``$($p.File)`` | $($p.Desc) | $($p.SizeMB) MB | ``$($p.Sha256)`` |"
}
$lines += @(
    "",
    "## 安装",
    "",
    "1. 下载 ``Scenecaps-$Version-full.zip``（或按需选择源码/文档包）",
    "2. 解压到 ``<你的项目>/Plugins/Scenecaps/``",
    "3. 重新启动编辑器并编译（需要 EngineVersion $((Get-Content (Join-Path $RepoRoot 'Scenecaps.uplugin') -Raw | ConvertFrom-Json).EngineVersion)）",
    "",
    "---",
    "",
    "生成时间: $((Get-Date).ToString('yyyy-MM-dd HH:mm:ss')) ($([DateTimeOffset]::Now.ToString('zzz')))"
)
Set-Content -LiteralPath $notesPath -Value $lines -Encoding utf8NoBOM

Write-Host ""
Write-Host "✅ 已生成 $($built.Count) 个包 + 发布描述" -ForegroundColor Green
$built | Format-Table File, SizeMB -AutoSize
Write-Host "发布描述: $notesPath"
