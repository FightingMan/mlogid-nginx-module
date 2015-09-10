Name
====
**mlogid** - trace request with hostname and random string

```
location ~ \.php$ {
    mlogid on;
    add_header X-Log-Id $mlogid;
    ...
}
```

