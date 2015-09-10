Name
====
**mlogid** - trace request with hostname and random string

```
./configure --add-module=/path/to/mlogid

location ~ \.php$ {
    mlogid on;
    add_header X-Log-Id $mlogid;
    ...
}
```

