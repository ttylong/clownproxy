# Clown Proxy

Clown Proxy is an assignment for CPSC 441 at the University of Calgary. It changes any instance of the word 'happy' regardless of casing to the word 'Silly', and changes any `.jpg` image to a picture of a clown. It works for plain text http.


## Requirements

 1. linux machine or a compiler such as cygwin
 2. C++17 or above

## Instructions

1. compile by typing `make` into the command line
    * alternatively, `g++ -Wall -O2 clownproxy.cpp -o clown` <p>
2. to run type in `./clown <portnum>` where `<portnum>` is the port number the proxy is to run on<p>
3. set browser settings to run a proxy on http. ensure the ip address is the same as the ip you are running the code on<p>
4. this proxy selects a random local clown image from the same directory
    * by default, the number of clown images it will choose is 3. This can be changed by changing the MAX_IMG_ID constant
    * clown images must follow the following naming format: `clownpicX.jpg` where X is an integer
    * the clown images MUST be labelled in ascending order, beginning at 1 and increasing by 1 (ie, clownpic1.jpg, clownpic2.jpg)


## Notes 
1. This assignment was designed around the assignment instructions, that is, it was tested only on the required http sites, and may not work for other sites
    * The web page it was tested on is: http://pages.cpsc.ucalgary.ca/~carey/CPSC441/ and any sub directory <p>
