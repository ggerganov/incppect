# hello-browser-ssl

Basic SSL example. Make sure to have valid certificate files.
Quick way to generate self-signed certificate is something along the lines of:

```
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout key.pem -out cert.pem
```

<a href="https://i.imgur.com/m3tKqxC.jpg" target="_blank">![hello-browser](https://i.imgur.com/m3tKqxC.jpg)</a>
