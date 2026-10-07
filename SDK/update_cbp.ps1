param([string]$ProjectFile = "JL380N.cbp")

$sourceDir = "apps/watch/ui/lvgl_ui_app/lvgl_v9/dashboard_1024x600_v1"
$includeDirs = @(
    "$sourceDir",
    "$sourceDir/custom",
    "$sourceDir/generated",
    "$sourceDir/generated/gui_events",
    "$sourceDir/generated/gui_fonts",
    "$sourceDir/generated/gui_group",
    "$sourceDir/generated/gui_images",
    "$sourceDir/generated/gui_msg",
    "$sourceDir/generated/gui_res",
    "$sourceDir/generated/gui_scr",
    "$sourceDir/generated/gui_timelines",
    "$sourceDir/generated/i18n"
)
$extensions = @(".c", ".cpp", ".cxx", ".cc", ".h", ".hpp", ".hxx", ".hh")

# 获取所有要添加的文件（相对于项目目录的路径）
$allFiles = Get-ChildItem -Path $sourceDir -Recurse -File | Where-Object { $extensions -contains $_.Extension }
$newFiles = @()
$existingFiles = @()
if (Test-Path $ProjectFile) {
    $content = Get-Content $ProjectFile -Raw -Encoding Default
    [regex]::Matches($content, '<Unit filename="([^"]+)"') | ForEach-Object { $existingFiles += $_.Groups[1].Value }
    foreach ($f in $allFiles) {
        $relPath = $f.FullName.Substring((Get-Location).Path.Length + 1).Replace('\', '/')
        if ($existingFiles -notcontains $relPath) {
            $newFiles += $relPath
        }
    }
} else {
    Write-Host "错误: 找不到项目文件 $ProjectFile"
    exit 1
}

# 添加新文件
if ($newFiles.Count -gt 0) {
    $content = Get-Content $ProjectFile -Raw -Encoding Default
    $unitLines = ""
    foreach ($f in $newFiles) {
        $unitLines += "`t`t<Unit filename=`"$f`" />`n"
    }
    $newContent = $content -replace '(?=</Project>)', $unitLines
    [System.IO.File]::WriteAllText($ProjectFile, $newContent, [System.Text.UTF8Encoding]::new($false))
    Write-Host "已添加 $($newFiles.Count) 个文件"
} else {
    Write-Host "没有新文件需要添加"
}

# 添加头文件目录
$existingDirs = @()
[regex]::Matches($content, '<Add directory="([^"]+)"') | ForEach-Object { $existingDirs += $_.Groups[1].Value }
$newDirs = $includeDirs | Where-Object { $existingDirs -notcontains $_ }
if ($newDirs.Count -gt 0) {
    $content = Get-Content $ProjectFile -Raw -Encoding Default
    $dirLines = ""
    foreach ($d in $newDirs) {
        $dirLines += "`t`t`t<Add directory=`"$d`" />`n"
    }
    # 在每个 </Compiler> 前插入
    $newContent = $content -replace '(?=</Compiler>)', $dirLines
    [System.IO.File]::WriteAllText($ProjectFile, $newContent, [System.Text.UTF8Encoding]::new($false))
    Write-Host "已添加头文件目录: $($newDirs -join ', ')"
} else {
    Write-Host "头文件目录均已存在"
}
$makefile = "Makefile"
$newLines = @'
ifneq ($(wildcard build/genUIFileList.c),)
	$(info +CC build/genUIFileList.c)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P build/genUIFileList.c >> build/fileList.mk
	cmd /c call build/append_ui_include.bat
endif
'@

# 使用 UTF-8 编码（无 BOM），与原始文件一致
$encoding = [System.Text.Encoding]::UTF8

# 读取文件
$content = [System.IO.File]::ReadAllText($makefile, $encoding)

$marker = "ifneq (`$(`wildcard build/genUIFileList.c`),)"
$escapedMarker = [regex]::Escape($marker)
if ($content -match $escapedMarker) {
    Write-Host "新内容已存在于 Makefile 中，跳过插入。"
    exit
}

$regex = '(pre_build:\s*\n)((?:\t.*\n)*)'
if ($content -match $regex) {
    $newBlock = $newLines -replace "(?m)^", ""
    $newContent = $content -replace $regex, "`$1`$2$newBlock`n"
    # 写入文件，保持 UTF-8 无 BOM
    [System.IO.File]::WriteAllText($makefile, $newContent, $encoding)
    Write-Host "已更新 Makefile"
} else {
    Write-Host "未找到 pre_build: 目标"
}