param([string]$HubRoot='', [ValidateSet('All','Execution')][string]$Suite='All')
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if (!$HubRoot) { $HubRoot=Join-Path (Split-Path $root -Parent) '#READ_ME_FIRST#' }
. (Join-Path $HubRoot 'scripts/invoke_hub_test.ps1')
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$msbuild=& $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'MSBuild with C++/MFC is required.' }
$engine=Join-Path $root 'build/srec_cat.exe'
if (!(Test-Path -LiteralPath $engine -PathType Leaf)) { throw 'Place the existing srec_cat.exe in build before verification.' }
$owned=[Collections.Generic.List[object]]::new()
function Invoke-Owned($Run,[string]$Executable,[string[]]$Arguments,[string]$Label) {
    $stdout=Join-Path $Run.Work ($Label+'.out.log')
    $stderr=Join-Path $Run.Work ($Label+'.err.log')
    $info=[Diagnostics.ProcessStartInfo]::new()
    $info.FileName=$Executable
    $info.Arguments=$Arguments -join ' '
    $info.UseShellExecute=$false
    $info.CreateNoWindow=$true
    $info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $info.RedirectStandardOutput=$true
    $info.RedirectStandardError=$true
    $process=[Diagnostics.Process]::new()
    $process.StartInfo=$info
    if (!$process.Start()) { throw "$Label did not start." }
    $owned.Add($process)
    $outputTask=$process.StandardOutput.ReadToEndAsync()
    $errorTask=$process.StandardError.ReadToEndAsync()
    if (!$process.HasExited) { Register-HubTestProcess -Run $Run -ProcessId $process.Id }
    if (!$process.WaitForExit(120000)) { throw "$Label exceeded the bounded test duration." }
    $process.WaitForExit()
    [IO.File]::WriteAllText($stdout,$outputTask.Result)
    [IO.File]::WriteAllText($stderr,$errorTask.Result)
    Write-Host $outputTask.Result
    Write-Host $errorTask.Result
    if ($process.ExitCode -ne 0) { throw "$Label failed with exit code $($process.ExitCode)." }
}
$result=Invoke-HubOwnedTest -ProjectRoot $root -Purpose 'SRecord execution/staging regression' -MaxBytes 256MB -Action {
    param($run)
    $projects=if ($Suite -eq 'Execution') { @('ExecutionRegression') } else { @('ExecutionRegression','SizeRegression') }
    foreach ($name in $projects) {
        $out=Join-Path $run.Work $name
        [void][IO.Directory]::CreateDirectory($out)
        Invoke-Owned $run $msbuild @(('"'+(Join-Path $PSScriptRoot ($name+'.vcxproj'))+'"'),'/nologo','/v:minimal',
            '/t:Rebuild','/p:Configuration=Release','/p:Platform=x64',('/p:OutDir="'+$out+'/"'),
            ('/p:IntDir="'+$out+'/obj/"')) ('build-'+$name)
        $fixture=Join-Path $run.Work ($name+'-data')
        $arguments=if ($name -eq 'ExecutionRegression') { @('--run',('"'+$engine+'"'),('"'+$fixture+'"')) }
            else { @(('"'+$engine+'"'),('"'+$fixture+'"')) }
        Invoke-Owned $run (Join-Path $out ($name+'.exe')) $arguments ('run-'+$name)
    }
    $uiConfigurations=if ($Suite -eq 'All') { @('Release','Debug') } else { @() }
    foreach ($configuration in $uiConfigurations) {
        $out=Join-Path $run.Work ('UiExecutionRegression-'+$configuration)
        [void][IO.Directory]::CreateDirectory($out)
        Invoke-Owned $run $msbuild @(('"'+(Join-Path $PSScriptRoot 'UiExecutionRegression.vcxproj')+'"'),
            '/nologo','/v:minimal','/t:Rebuild',('/p:Configuration='+$configuration),'/p:Platform=x64',
            ('/p:OutDir="'+$out+'/"'),('/p:IntDir="'+$out+'/obj/"')) ('build-ui-'+$configuration)
        Invoke-Owned $run (Join-Path $out 'UiExecutionRegression.exe') @(('"'+$engine+'"'),
            ('"'+(Join-Path $run.Work 'ExecutionRegression/ExecutionRegression.exe')+'"'),
            ('"'+(Join-Path $run.Work ('ui-data-'+$configuration))+'"')) ('run-ui-'+$configuration)
    }
} -Teardown {
    param($run)
    foreach ($process in $owned) {
        if (!$process.HasExited) {
            # Only the retained Process object from this fresh runner is owned.
            $process.Kill()
            $process.WaitForExit()
        }
    }
} -CollectArtifacts {
    param($run)
    # These two fresh compiler/fixture locations and runner logs are the only producers.
    Register-HubArtifactTree -Run $run -Producer 'isolated MSBuild and SRecord execution regression runner'
}
$result | Select-Object Success,Outcome,CleanupRemoved,Errors,Output | Format-List
if (!$result.Success) { throw 'Execution verification or registered cleanup failed.' }
