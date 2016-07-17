# Force-It Box

While investigating the world of 'provably fair' online gambling I encountered one particularly poorly written Bitcoin faucet dice script which is popular in the micro-earning community because it's open-source, simple to setup and can be run on cheap web hosts.

 * https://github.com/coinables/Bitcoin-Faucet-Dice-Faucet-Box

The algorithm claims to be 'provably fair', but I have a feeling that the author didn't fully grok the theory behind it because there's no way to input client-side random into the roll so the server can still manipulate which numbers it picks, but that's not the main problem, check the source code which generates the hash:

```php
  //generate roll id
    $gameid = uniqid();
  //generate salt
  $genSalt = time();
  $genSalt2 = mt_rand(1111111, 3333333);
  $genSalt3 = $genSalt2 / 1000;
  $genSalt4 = $genSalt3 * $genSalt;
  $salt = sha1($genSalt4);
  $spacer = "+";
   //generate roll 
    $pick = mt_rand(0, 10000);
    
    $pick2 = $pick / 100;
    $proof = sha1($salt.$spacer.$pick2);

// ... *snip*

if($amount > 99999){
        die("Stop hacking you hacker");
```

So what's wrong with that?

 * Current time is returned in the `Date:` header
 * `time()` is always UTC
 * 1st random number range is 2222222
 * 2nd random number range is 10000

Yup, the search space contains about 22 billion entries with a known starting seed, at 250 chrono-ticks per second (equivalent to 2.5m SHA-1 digests) it can take several hours to brute force the hash with a single thread, which is a lot of spare time to be looking at the computer doing nothing, so during that time I wrote a work distributor that allowed me to spread the load across an arbitrary number of servers over SSH in addition to my local machine.

Now with a few more servers I can reduce the brute force time from 3 hours to 30 minutes, and with a few more it can be done in a handful of minutes. However, given the miniscule amount you can win from these sites by guessing the right number, the CPU power required to get anywhere quickly is cost-prohibitive.

## How to Use

 1. Compile on *BSD/Linux/Mac
 2. Refresh dice page (through proxy)
 3. Extract `Date` header from response
 4. Convert it using `strtotime.php`

Example:

```
make
php strtotime.php Sun, 17 Jul 2016 04:11:41 GMT
./forceitbox 1468728701 0f2107eb9113274f2f8e37df91165af73c8497e7
```

To distribute the brute-force work across more computers the `ghettomp.py` tool has been provided, you must edit the file and replace the demo servers (or comment out everything apart from `localhost` to start with).

Then run `ghettomp.py` as `forceitbox` was run above:

```
./ghettomp.py 1468728701 0f2107eb9113274f2f8e37df91165af73c8497e7
```

Now the work will be split into random chunks, shuffled and fed to all available servers, printing the result when ready.

## Links

 * http://matacoin.com/faucetboxgame/
 * http://digieuro.in/
 * http://btcbestco.in.net/faucetboxgame/
 * http://profitbyclick.tk/faucetboxgame/
 * http://www.bitcoinshowers.com/playbtcgame/
 * http://witchfaucet.com/faucetdice/
 * http://bitcoinmad.xyz/faucet/faucetboxgame/
 * http://crystalbtc.co/dice/faucetboxgame/
 * http://faucetcoinlist.com/faucetboxgame/
 * http://linkvay.tk/ltc/faucetboxgame/
 * http://btcbestco.in.net/faucetboxgame/
 * http://www.bitcoinshowers.com/playbtcgame/
 * http://bitcoinfaucetdice.com/


