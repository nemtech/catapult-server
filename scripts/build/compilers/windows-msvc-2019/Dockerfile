# escape=`
# source: https://github.com/thejohnfreeman/rippled-docker

FROM mcr.microsoft.com/dotnet/framework/sdk:4.8

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

# Download the Visual Studio Build Tools bootstrapper.
ADD https://aka.ms/vs/16/release/vs_buildtools.exe C:\\Temp\\vs_buildtools.exe

# Use the latest release channel.
ADD https://aka.ms/vs/16/release/channel C:\\Temp\\VisualStudio.chman

# For help on command-line syntax:
# https://docs.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio
# Install MSVC C++ compiler, CMake, and MSBuild.
RUN C:\\Temp\\vs_buildtools.exe `
    --quiet --wait --norestart --nocache `
    --installPath C:\\BuildTools `
    --channelUri C:\\Temp\\VisualStudio.chman `
    --installChannelUri C:\\Temp\\VisualStudio.chman `
    --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended `
    --add Microsoft.Component.MSBuild `
 || IF "%ERRORLEVEL%"=="3010" EXIT 0

# Install Python and Git.
# tls issue hack: https://stackoverflow.com/questions/60066567/chocolatey-install-error-the-request-was-aborted-could-not-create-ssl-tls-secu
RUN powershell.exe -ExecutionPolicy RemoteSigned `
  [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; `
  iex (new-object net.webclient).downloadstring('https://get.scoop.sh'); `
  scoop install python git

RUN python3 -m pip install -U "conan>=1.33.0"

# Start developer command prompt with any other commands specified.
ENTRYPOINT ["C:\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat", "&&"]
