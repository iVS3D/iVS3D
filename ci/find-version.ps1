$tag=git describe --tag --abbrev=0
$tag_long=git describe --tag

If("${tag}" -eq "${tag_long}"){
    Write-Output "${tag}"
} Else {
    Write-Output "${tag}-dev"
}