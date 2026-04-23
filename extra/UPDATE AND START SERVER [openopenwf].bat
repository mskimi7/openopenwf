@echo off

echo Updating SpaceNinjaServer...
git fetch --prune
if %errorlevel% == 0 (
	git restore package-lock.json
	git stash
	git checkout -f origin/main
	node -e "n=`src/controllers/api/loginController.ts`;f=require(`node:fs`);d=f.readFileSync(n,`utf8`);i=d.indexOf(`diapers`)-67;if(i>0){f.writeFileSync(n,d.substring(0,i)+d.substring(i+105))}"

	if exist static\data\0\ (
		echo Updating stripped assets...
		cd static\data\0\
		git pull
		cd ..\..\..\
	)

	echo Updating dependencies...
	node scripts/raw-precheck.js > NUL
	if %errorlevel% == 0 (
		call npm i --omit=dev --omit=optional --no-audit
		call npm run raw
	) else (
		call npm i --omit=dev --no-audit
		call npm run build
		if %errorlevel% == 0 (
			call npm run start
		)
	)
	echo SpaceNinjaServer seems to have crashed.
)

:a
pause > nul
goto a
