# DefToLib.ps1
# A .lib generation script for undocumented DLL exports
# https://github.com/aubymori/DefToLib
# 
# Arguments:
# -machine: The machine type to generate a LIB for. "X86" by default.
# -def:     The DEF file.
# -outdir:  The output directory
#
# The resulting .lib file will be at ($outdir)/($def).lib.
#   e.g. "DefToLib.ps1 -def somedll.def -outdir C:\libs" -> "C:\libs\somedll.lib"

param(
    [string]$machine = "X86",
    [Parameter(Mandatory=$true)][string]$def,
    [Parameter(Mandatory=$true)][string]$outdir
)

# Workaround for a PowerShell bug involving argument parsing
# https://github.com/PowerShell/PowerShell/issues/4358
if ($outdir.Remove(0, $outdir.Length - 1) -eq '"')
{
    $outdir = $outdir.Remove($outdir.Length - 1) + ""
}

$defnoext = $def.Substring(0, $def.lastIndexOf(".")).Substring($def.lastIndexOf("\") + 1)

# We need to specially handle X86 because of __stdcall decorations.
if ($machine -eq "X86")
{
    $cpptext = "int __stdcall DllMain(struct HINSTANCE__*,unsigned long,void*){return 0;}"
    $deftext = Get-Content -Path $def
    $defsplit = ($deftext.Replace("`r`n", "`n")) -split "`n"

    $hitexports = $false
    $libname = ""

    for ($i = 0; $i -lt $defsplit.Count; $i++)
    {
        $line = $defsplit[$i]

        if ($line.Trim().StartsWith("LIBRARY"))
        {
            $libstart = $line.lastIndexOf(" ")
            if ($libstart -eq -1)
            {
                continue
            }
            $libname = $line.Substring($libstart + 1)

            if ($libname.Substring($libname.Length - 4) -eq ".DLL")
            {
                $libname = $libname.Substring(0, $libname.Length - 4)
            }

            Write-Output "Library: $libname"
        }

        if ($hitexports -eq $true)
        {
            $line = $line.Trim()
            $funcend = $line.IndexOf(" ")
            if ($funcend -eq -1)
            {
                $funcend = $line.IndexOf(";")
                if ($funcend -eq -1)
                {
                    Write-Output "Ignoring line with no spaces or semicolons"
                    continue
                }
            }

            $funcname = $line.Substring(0, $funcend)

            $commentstart = $line.IndexOf(";")
            if ($commentstart -eq -1)
            {
                Write-Output "Ignoring line with no comment"
                continue
            }

            $comment = ($line.Substring($commentstart + 1)).Trim()
            if (-not $comment.StartsWith("argsize="))
            {
                Write-Output "Ignoring line with no argsize= comment"
                continue
            }

            try
            {
                $argsize = [int]($comment.Substring("argsize=".Length))
            }
            catch
            {
                Write-Output "Argsize in comment '$comment' is not an integer"
                continue
            }

            if ($argsize -lt 0)
            {
                Write-Output "Ignoring negative arg size $argsize"
                continue
            }

            # 0 fits this condition too.
            if (-not ($argsize % 4) -eq 0)
            {
                Write-Output "Ignoring arg size $argsize which is not divisible by 4"
                continue
            }

            $argc = $argsize / 4
            $cpptext += "`r`n"
            $cpptext += 'extern "C"'
            $cpptext += " void __stdcall $funcname("
            for ($j = 0; $j -lt $argc; $j++)
            {
                $cpptext += "int"
                if (-not ($j -eq ($argc - 1)))
                {
                    $cppText +=","
                }
            }
            $cpptext += "){}"
        }
        elseif ($line.Trim() -ceq "EXPORTS")
        {
            $hitexports = $true
        }
    }

    $outcpp = Join-Path $outdir "$defnoext.cpp"
    Write-Output $cpptext | Out-File $outcpp

    $outdll = Join-Path $outdir "$libname.dll"
    $outlib = Join-Path $outdir "$defnoext.lib"
    cl.exe "$outcpp" "/link" "/DLL" "/DEF:$def" "/MACHINE:$machine" "/ENTRY:DllMain" "/OUT:$outdll" "/IMPLIB:$outlib"
}
# For X64 and ARM64 we can just compile the lib like normal
else
{
    $outpath = Join-Path $outdir "$defnoext.lib"
    lib.exe "/NOLOGO" "/DEF:$def" "/MACHINE:$machine" "/OUT:$outpath"
}