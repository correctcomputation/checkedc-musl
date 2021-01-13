# Description 

For the purposes of CCI engineers, this fork of Microsoft's 
conversion of MuSL serves to 1) serve as a means of testing 
new array bounds inference protocols for 5c and 2) also examine
exactly how it is done by previous MS interns. 

For our purposes, the bulk of the information is present in the 
src/network directory, and this README will include a running, 
updated list of all the files that contain some interesting array
bounds inference related stuff in it. These files may also contain 
comments of the form `CCI BOUNDS` with content that may be helpful
to us as we develop.  

In order to view the file-diffs properly, navigate to the file and 
toggle between `master` and `baseline_0629`

# File-diffs to examine

- `dn_comp.c` 
