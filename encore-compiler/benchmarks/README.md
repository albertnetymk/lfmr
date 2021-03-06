### config.rb:

Configuration file for benchmarks, specify benchmarks to be
run and parameters to them in this file.
        
All benchmarks must be of the following format: 

```
[benchname: string, confList: list]

```

, where `benchname` is the name of the
benchmark (must be the same name as the corresponding `.rb` file
in `/rbfiles`, excluding the `.rb` extension) and `confList` is a
list of tuples (more precisely a list of lists, since Ruby
doesn't support tuples by default) with parameters for the
benchmark.

each element of `confList` must have the following form:

```
[description: string, [arg1, arg2, ..., argn]: list]
```                 
```
eg.      ["2000 meetings, 20 creatures", [2000, 20]]
```

The benchmark will be run REPS time for each configuration in
`confList`, the result is then stored in `/data` in the following
format: `benchname-description.data`.

A summary of all runs is located at the end of the file. The
shortest measured execution time and the average memory
consumption over all runs is found in the summary.


### /rbfiles (folder):
Each `.rb` file in `/rbfiles` must define a function named
`sample` that accepts an array as input argument. This array
contains the arguments specified in the corresponding `confList`.

Each file also implements an optional `expected` function
which takes the same array as `sample` and returns an expected
output given the input in `args`. The return value should be an
expected substring of the program output.

One example file is given and is located at
`/rbfiles/example.rb`. For an example with a defined `expected`
function, see `/rbfiles/pl_chameneos_encore.rb`.

Each `.rb` file must execute the corresponding program with the
contents of `arr` as command line parameters in the sample
function. To do this, send the path to the program's folder
and the unix command to be executed to the `execute` function
located in `helper.rb` (no imports need it, just call
`execute(path, command)` from the `.rb` file as shown in
`/rbfiles/example.rb`) and return the result of the `execute`
function.

### run.rb:
The main file for this piece of software, parses input
parameters and runs and parses the output from `runner.rb`.

##### runner.rb: 
Runs the sample function of the current `.rb` file (which is
included by `run.rb` when calling `runner.rb`) and checks whether
or not the output of sample is the same as the expected output
(if the `expected` function is defined in the current `.rb`
file).

##### helper.rb 
Inlcudes the `execute` function which every `.rb` file should
call. Executes the command passed from the `.rb` file and
returns the output and time measurements of the program.

**Use:**

Run `ruby run.rb`. This will run all benchmarks that are assigned
to the `BENCHMARKS` constant in `config.rb`.

**Flags:**

It's possible to add different flags to `run.rb`.
Syntax: 

```
ruby run.rb -FLAGNAME PARAMETER
```

All flags with their descriptions are listed below:

```
-p [THREADS]     # Sets ENV['ponythreads'] to THREADS

-t [TAG]         # prepends TAG to all generated data files, can be
                   used to avoid replacing old results

-s               # runs benchmarks in silent mode

-v               # runs benchmarks in verbose mode
```

**Shortcomings:**
 
* no error handling. If a program crashes or is not able to
  execute for some reason, this will not be visible in
  the data files containing the measured execution time 
  unless an `expected` function is defined in the corresponding 
  `.rb` file.

  **Improvement suggestion**: inspect result of `stderr` in
  `helper.rb` and handle errors from there.

* `config.rb` is a bit cluttered and could probably be
  improven in that aspect. 

  **Improvement suggestion**: includes removing the configuration
  name requirement since it's redundant in many cases.

* graph generation is missing. 

  **Improvement suggestion**: create script that uses
  *gnuplot* to parse the datafiles.

* author of benchmark `.rb` files must locate and execute the
  benchmark programs. It would (probably) be preferable to
  execute all encore programs in a generic way instead of
  trusting the author of the benchmark `.rb` files to follow
  some convention.

  **Improvement suggestion**: author of files in `/rbfiles`
  only specifies path to program and what language the program
  is written in, then this program is executed automatically
  in a generic way by some ruby program.

* the `-p` flag doesn't enforce that encore benchmarks uses the
  specified amount of threads unless the author of the
  corresponding benchmark `.rb` files executes the encore
  programs with `ENV['ponythreads']` as thread amount. (as in
  `example.rb`)

  **Improvement suggestion**: execute all encore programs in a
  generic way specified by some ruby program. Only require the
  author of benchmark `.rb` files to specify a path to the
  benchmark program and let the ruby program execute it, this
  way one can enforce that the correct amount of *ponythreads*
  are used.

### Preliminaries:
The benchmark suite is tested on:

* `ruby 1.9.3`