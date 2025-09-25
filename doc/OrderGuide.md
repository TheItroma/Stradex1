# Component buying guide

### Tools

For this build, you'll need : 

- A 3d printer (I use an ender 3 pro, Very worth for the price)
- A soldering kit (Literally any cheap one will do, if you have more money, Look around for separated higher qualitie items but I don't suggest investing in good stuff since the cheap stuff will last you long and be absolutly fine. If you do want the higher quality stuff, you'll need to buy a solder wick or pump and a solder cleanning spunge or brass ball)
- Solder (**DO NOT USE THE ONE PROVIDED WITH THE KIT** from my experience, its shit (its ok if you're really on a budget but do at your own risk). I'd reccomend buying it separatly, make sure its lead free(ideally) and that it has a good % of flux - I use 2%. Also, the size should be ~ 0.5mm - 1mm)
    
Optional but nice to have : 

- Helping hand (NO NEED TO BUY, you can made ones out of laundry clips or just 3d print 1)
- Flux (idk, in case you have a solder with few of it or if u wanna make em better and prettier)

### **IF YOU'RE HESITANT ON BUYING A 3D PRINTER :**

In my oppinion, every single person on this earth should own one. For all the shitty plastic gadget you buy for whatever reason, theres an INSANE transportation pollution and monetary cost associated. Its all imported from china and NOT AT ALL space efficient. Owning a 3d printer allows you to localise the manifacturing for plastic recyclable usefull parts that you can design yourself. It allows you to repair expensive stuff that really only needs a tiny tiny plastic part to function that the manifacturer won't provide. It allows you to make a whole ass organisation system for every aspect of your life - Checkout gridfinity. If you are passionate about music making and broke, chances are buying a 300 CAD printer will allow you to make multiple instruments costing 100 CAD but as good if not more than 599 CAD consumer versions (The synth/midi community is kinda getting riped off lol).

**Also, you can always send the parts to be printed to places [like pcbway](pcbway.com) or a local FAB(Fabrication places(they have 3d printers, cnc stuff, solder kits, etc... all for rent))**

### The pcb

There are 2 main platforms for ordering pcb's : jlcpcb and pcbway. From what It says online, jlcpcb.com is the better platform for pcb's. So go to jlcpcb.com, scroll down and "add gerber files" go to the folder you downloaded -> SDX_gerbers -> The latest zip file (idk why he has multiple versions considering github exists but oh well). You do not need to extract it. You cannot select more than 5 pcb's sooo, make some for ur friends or sell them or smth. Here are the only settings you might want to change : PCB Color (carefull with that, the price might fluctuate per demand), Surface finish (Pick LeadFree, Its very slightly harder to solder, very slightly more expensive but doesn't contain lead so more environement friendly). Then, select your shippement option and build time and done.

**A PCB-LESS VERSION MIGHT BE DESIGNED**

### The components

You'll need :
    
| Part       | Quantity for 1  | Quantity in sold package  | Notes   |Price (CAD)  | Url |
| --------------------|-----------------:|----------------:|------------|----:|-----|
| Raspberry pi pico |     1 |  1|  Choose the version without header | 7.8 | https://www.pishop.ca/product/raspberry-pi-pico-2/?src=raspberrypi|
| SoftPot 200mm (SP-L-0200-103-3%-RH) | 1 | 1 | | 40 | https://www.digikey.ca/en/products/detail/spectra-symbol/SP-L-0200-103-3-RH/2175427 |
|  ADS1115 Breackout boards | 2 | 2 | | 5 | https://www.aliexpress.com/item/1005007906475242.html |
| 10K Vertical Potentiometer | 3 | 5 | | 3.70 | https://www.aliexpress.com/item/1005006220604488.html |
| 10k FSR Soft tactile pushbuttons | 3 | 100 | | 5 | https://www.aliexpress.com/item/1005009964547709.html |
| 10k Resistors | 10 | 300 | | 5 | https://www.aliexpress.com/item/1005008321368077.html |
| M3 40mm Machine screws | 4 | 20 | | 5 | https://www.aliexpress.com/item/33003801934.html |
| Other M3 Machine screws | 20 | 392 | | 8.50 | https://www.aliexpress.com/item/1005007273134914.html |

**VERY ROUGHT ESTIMATE, YMMV, INCLUDES TAXES AND SHIPPING** Assuming you already have a 3d printer and fillament, a soldering iron and some salvaged wire or bought off aliexpress.

**Total price : 80 CAD**
