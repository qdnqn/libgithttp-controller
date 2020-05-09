
# libgithttp-controller
This program is intended for use with the library [libgithttp](https://github.com/qdnqn/libgithttp) for allowing brokering service.

## Building program
```
# Assuming you have libgit2 compiled in directory ../libgit2/build and
# hiredis in directory ../redis/ 
# and lua-dev 5.3 installed

git clone https://github.com/qdnqn/libgithttp-controller
cd libgithttp-controller
make
```
#### Running and configuration
For configuration refer yourself to gc_config.h. To run this program just type
```
./git_controller
```
#### Dependencies

1. Installed mongo and redis database
2. mongoc
3. libbson
4. lua5.3-dev
5. hiredis

#### Brokering
When events of pull or push occur the git_controller detects them by listening on all redis channels using psubscribe for matching with pattener ```*.repo``` so it can distinguish different repos. Broker receives data containing about specific actions so we can put it to some use and save it to database. Database used is mongo. 



Broker is using async redis and it's listening forever with psubscribe. Aside from logging broker can activate triggers defined in redis database. Triggers are LUA scripts. This program has ability to embed lua function and call it from itself.



Example of trigger function(LUA script):
```
function trigger(str)
    local json = require "dkjson"
    
    print("Printing data from lua!")
    print(str)

    local decode = json.decode( str )
    print(decode)

    print("Going back to C now!")
end
```
To set trigger for some repo you just need to save it in redis as
```
SET reponame.repo:trigger "HERE GOES CODE ABOVE"
```
function defined in lua needs have definition ```function trigger(str)``` so It can be called properly.
After event occurs broker is trying to find trigger in redis. If found it's going to call trigger(str) function.

str used in lua script is string of json data containing data passed from [libgithttp](https://github.com/qdnqn/libgithttp).
When push is made, message received can be like:

```json
{
   "message":"Pushing to repository.",
   "user":"test",
   "repo":"test",
   "path":"/path/to/repo",
   "type":"push",
   "ref_oid":"refs/heads/master",
   "old_oid":"6bca08ae530b8e9eaac62d408c4f0b3a2826f7ea",
   "new_oid":"bdd6ceb415a8f5cfbecf0ed64310309c7880e1b8",
   "indexed_obj":"6",
   "_id":{
      "$oid":"5eb659bfd4903b270b688b27"
   },
   "timestamp":{
      "$timestamp":{
         "t":1589008831,
         "i":0
      }
   }
}
```
There is numerous ways what you can do with this information provided. Only limitation is given by LUA itself. As you can see triggers are very extensible and you can customize your triggers to every situation you can find yourself in.


## License
Copyright 2020 adnn.selimovic@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

