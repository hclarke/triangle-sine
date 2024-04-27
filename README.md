# Δ~

draws a triangle, plays some sine waves

## set up env

if you already have cosmocc and usual linux tools on PATH, you can maybe skip this

```
cosmo-env/build.sh # or .ps1
cosmo-env/activate.sh # or .ps1
```

## build

```
make o/triangle-sine.com
```

## run

with system SDL2

```
o/triangle-sine.com
```

with included SDL2 (windows-only, for now)

```
SDL2_DYNAMIC_API=/zip/SDL2.dll o/triangle-sine.com
```

