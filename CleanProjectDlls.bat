@echo off
rmdir /q/s "Build"
rmdir /q/s "Binaries"
rmdir /q/s "Cooked"

for /d /r "Plugins/" %%X in (*"Binaries") do (
  pushd "%%~X"
  if exist %%X\"Win64" rmdir /q/s "%%~X\Win64"
  popd
)