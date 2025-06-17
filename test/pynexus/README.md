
## Example to generate kernel.so


To generate LLVM IR:
```script
xcrun -sdk macosx metal -c kernel.metal -o kernel.ir
llvm-dis kernel.ir -o kernel.ll
```

To generate SO library binary:
```script
xcrun -sdk macosx metal -c kernel.metal -o kernel.ir
xcrun -sdk macosx metallib kernel.ir -o kernel.so
```
