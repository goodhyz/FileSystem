# 创建一个空的文件
$filePath = "MyDisk.dat"
New-Item -ItemType File -Path $filePath -Force

# 计算100MB的字节数
$mbInBytes = 100 * 1024 * 1024

# 创建一个100MB大小的字节数组, 并写入文件
$buffer = New-Object byte[] $mbInBytes
$null = Set-Content -Path $filePath -Value $buffer -Encoding Byte