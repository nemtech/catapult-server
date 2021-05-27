# escape=`

FROM {{BASE_IMAGE}}
MAINTAINER Catapult Development Team
SHELL ["cmd", "/S", "/C"]

RUN powershell.exe -ExecutionPolicy RemoteSigned `
  iex (new-object net.webclient).downloadstring('https://get.scoop.sh'); `
  scoop install python git; `
  python3 -m pip install --upgrade pip; `
  python3 -m pip install --upgrade colorama cryptography gitpython ply pycodestyle pylint pylint-quotes PyYAML

CMD ["powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
