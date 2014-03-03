def intrepr(size,precision=2):
    suffixes=['B','KiB','MiB','GiB','TiB']
    suffixIndex = 0
    while size > 1024:
        suffixIndex += 1
        size = size/1024.0
    return "%.*f %s"%(precision,size,suffixes[suffixIndex])