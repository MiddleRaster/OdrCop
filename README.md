# OdrCop

A tool to detect ODR violations, based on DIA so it only works with binaries build with the MSVC toolset.

## Usage Details

Usage: ```OdrCop <path to folder of .obj/.pdb files> [more folders ...]``` where each .obj/.pdb is compiled with /Zi, /Ob0 and /Fd$(IntDir)%(Filename).pdb

Build your .dll or .exe with whatever options you like except that the following **must** be set:
1. /Zi == use Debug info
2. /Ob0 == turn off inlining
3. /Fd == output a .pdb file for each .obj, using /Fd$(IntDir)%(Filename).pdb under Configuration Properties -> C/C++ -> Output File -> Program Database File Name
4. pass one or more paths to folders of .obj and .pdb files (N.B.: the tool will recurse into subfolders)

## How it works

For user defined types, I use the DIA SDK to read the .pdb file. DIA is all COM objects so make sure you have done "regsvr32 msdiaL40.dll" on your machine.  
For functions, I wrote my own COFF reader, which reads right from the .obj files (I can't use DIA because the linker throws away too much information about inlines).  

The output looks something like this, for a ```struct DifferentBases```, a ```struct DifferentConstDataMember``` and a function named ```FunctionsMustBeBitwiseIdentical```:
```
Loading: "TUs\\x64\\Debug\\dllmain.pdb"
Loading: "TUs\\x64\\Debug\\TU1.pdb"
Loading: "TUs\\x64\\Debug\\TU2.pdb"

ODR VIOLATION: DifferentBases
  [TUs\x64\Debug\TU1.pdb]
    kind=struct  size=1
    bases: Base1
  [TUs\x64\Debug\TU2.pdb]
    kind=struct  size=1
    bases: Base2

ODR VIOLATION: DifferentConstDataMember
  [TUs\x64\Debug\TU1.pdb]
    kind=struct  size=4
    +0 const  int32_t  a
  [TUs\x64\Debug\TU2.pdb]
    kind=struct  size=4
    +0  int32_t  a


ODR VIOLATION: FunctionsMustBeBitwiseIdentical
   [TUs\x64\Debug\TU1.obj]
   function body length: 9
   the first few bytes are: 40 57 b8 01 00 00 00 5f c3 
   [TUs\x64\Debug\TU2.obj]
   function body length: 9
   the first few bytes are: 40 57 b8 02 00 00 00 5f c3 

3 ODR violation(s) found.
```


## Known limitations of DIA

Some things are not recorded in the PDB debug info, making some ODR violations undetectable by DIA.

Here are some known undetectable ODR violations that I've tested:
   - Access specifier differences (public/private/protected)
   - Default argument differences
   - static constexpr / static const member value differences
   - static constexpr / consteval / constinit

Below here are supposedly undetectable by DIA, but I haven't tested (yet):
   - constexpr / inline differences on member functions
   - noexcept differences
   - Attribute differences (__declspec etc.)

## Known limitations of my COFF reader

I'm currently comparing function bodies byte-by-byte on the codegen, so it's not a *symantic* ODR check.  
For example,
```inline void Foo() { return 2+2; }``` vs ```inline void Foo() { return 4; }``` is an ODR violation, but the codegen is identical, so I can't tell.

Other things are just not implemented yet:  
Relocation canonicalization:  external symbols, for example, are not fixed up (like the linker does).  

## Final notes
See ```TUs\TestHeader.h``` for a list of ODR violations I haven't tested/implemented yet.  
If you can think of an ODR violation that's not on my TODO list, let me know.  
