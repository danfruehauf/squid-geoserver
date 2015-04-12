# Squid & Geoserver

At some point during my job I had to configure an efficient setup of Squid
(or another reverse cache proxy) caching dynamic WMS-C queries in front of
Geoserver. Unfortunately I could not find any good guide at the time, so I
decided it might be a good idea to have everything needed in one repository.

This setup is recommended mainly for tiled clients, such as OpenLayers.

## Objective

In the original setup I had something like:
```
/--------------------\     /---------------------\     /----------\
| Apache + mod_proxy | --> | Geoserver in Tomcat | --> | Postgres |
\--------------------/     \---------------------/     \----------/
```

My aim is to get you to have something like:
```
/--------------------\     /-------\     /---------------------\     /----------\
| Apache + mod_proxy | --> | Squid | --> | Geoserver in Tomcat | --> | Postgres |
\--------------------/     \-------/     \---------------------/     \----------/
```

Having squid serving portions of your tiled WMS-C requests can significantly
reduce load on your database backend and also improve performance by a few
orders of magnitude.

## Setup Squid

By default, squid will have a few things that will need tweaking. To begin with
squid will not use query terms as part of the query, so the 2 following queries
will be treated the same:
 * http://localhost:8080/geoserver/wms?query1
 * http://localhost:8080/geoserver/wms?query2

In order to stop that from happening, Have the following snippet in squid:
```
# This line comes as default on many squid configurations - REMOVE IT OR COMMENT IT!
# hierarchy_stoplist cgi-bin ?
# Tell squid to not strip query terms
strip_query_terms off
```

Last but not least, our `refresh_pattern`, which will need a few extra parameters:
```
# Tell squid to cache any urls which has '/wms?' in them
# 720 stands for 6 hours, tweak it as you see fit
# override-expire and ignore-reload are needed as geoserver will suggest to not
# cache dynamic tiles coming from geoserver
refresh_pattern http://localhost:8080/geoserver/(.+/)?wms\?.* 720 100% 720 override-expire ignore-reload
```

So to summarize, this is what you will need in `squid.conf`:
```
# hierarchy_stoplist cgi-bin ?
strip_query_terms off
refresh_pattern http://localhost:8080/geoserver/(.+/)?wms\?.* 720 100% 720 override-expire ignore-reload
```

### Setup Apache Web Server

Assuming you're using `mod_proxy` to forward tile requests to geoserver, the
following line in your virtual host will ensure you go through squid for
geoserver requests:
```
ProxyRemote http://localhost:8080/geoserver http://localhost:3128
```

A full vhost might look like:
```
<VirtualHost *:80>
  ServerName www.geoserver.com
 
  LogLevel info
  ErrorLog /var/log/apache2/www.geoserver.com-error.log
  CustomLog /var/log/apache2/www.geoserver.com-access.log combined
 
  ProxyPreserveHost On
  ProxyRemote http://localhost:8080/geoserver http://localhost:3128
 
  ProxyPass           /geoserver http://localhost:8080/geoserver
  ProxyPassReverse    /geoserver http://localhost:8080/geoserver
 
  RewriteEngine On
  RewriteOptions inherit
  RewriteLog /var/log/apache2/www.geoserver.com-rewrite.log
  RewriteLogLevel 0
</VirtualHost>
```

Have a look at https://bashinglinux.wordpress.com/2014/04/27/apache-squid-tomcat/
should you need more help. I assume you have good familiarity with setting up
Apache Web Server and Tomcat.

At this point, squid should intercept tile requests and cache them, **GOOD JOB!**

## Tile Seeding (optional)

For tiled clients, (like OpenLayers.js) it is possible to seed tiles before
they are being used. There is a utility I wrote which factors OpenLayers tiles
and can be obtained from the following repository:
https://github.com/aodn/utilities/tree/master/geoserver/geoserver_seeder

It contains usage instructions. Basically what it does is generate heaps of
URLs and run them through `squidclient` to `PURGE` and then `GET` them.

Using this utilitiy requires familiarity with how tiled layers work and also
understanding what zoom levels are.

### Store ID (optional) - Squid 3.4 and later!!

**This step is completely optional and is not required if you are using the same
tile client which produces the same query string every time.**

Unfortunately squid is not intelligent enough when it comes to handling WMS-C
requests, or requests in general. Consider you have these two supposedly
identical tile requests:
 * http://localhost:8080/geoserver/wms?SERVICE=wms&REQUEST=GetMap&VERSION=1.3.0&BBOX=110,-50,160,-3&FORMAT=image/png&WIDTH=10&HEIGHT=10&LAYERS=some_layer
 * http://localhost:8080/geoserver/wms?REQUEST=GetMap&SERVICE=wms&VERSION=1.3.0&BBOX=110,-50,160,-3&FORMAT=image/png&WIDTH=10&HEIGHT=10&LAYERS=some_layer

Don't see the difference? I just shuffled the order of the parameters, those
two requests obviously refer to the same tile.

In order to make those two requests (and more) evaluate to the correct tile, we
have to modify the way squid is storing objects - modify their store id.

Luckily, I have created a program to parse WMS-C requests and align them so
they cache better. In order to install it, use:
```
$ (cd wms-store-id && sudo make install)
```

Add the following line to `squid.conf`:
```
store_id_program /usr/local/bin/wms-store-id
```

Restart squid and you're good to go.

