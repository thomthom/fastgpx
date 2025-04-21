
# GPX

## GPX for Developers

https://www.topografix.com/gpx_for_developers.asp

> The standard in the gpx files for the time zone format is not completely
> clear. For instance some applications generate time formats in the track
> points of the form
>
> <time>2008-07-18T16:07:50.000+02:00</time>
>
> this is slightly substandard because the standard as described in
> http://www.topografix.com/gpx.asp says that
>
> "Date and time in are in Univeral Coordinated Time (UTC), not local time!
> Conforms to ISO 8601 specification for date/time representation."

https://web.archive.org/web/20130725164436/http://tech.groups.yahoo.com/group/gpsxml/message/1090?l=1

## GPS Exchange Format (GPX): A Comprehensive Guide

https://mapscaping.com/gps-exchange-format-gpx/

## Assumed typical <time> representation in .GPX files

To simplify logic and maximize performance, maybe one can assume a more narrow
scope of ISO 8601.

By using the length of the string one can make assumption to the variation of
the format and directly extract the numeric components of the string. Probably
worth validating the separator characters as a minimal validation safety check
to eliminate pure junk input.

Variations observed:
* Only Extended Format.
* With or without milliseconds. (3 fractional decimals)
* Zulu hours are the norm, but timezone offsets might occur.
* Some mention of missing timezone notation all together.
  While ISO 8601 describe this as local time, one can probably assume this is
  an omission by the author and it really should be Zulu hours.

| Example                       | Length |                                     |
|-------------------------------|--------|-------------------------------------|
| 2008-07-18T16:07:50.000+02:00 |     29 | Not per GPX definition.             |
| 2008-07-18T16:07:50.000Z      |     24 |                                     |
| 2008-07-18T16:07:50+02:00     |     25 | Not per GPX definition.             |
| 2008-07-18T16:07:50Z          |     20 |                                     |
| 2008-07-18T16:07:50           |     19 | Assume Zulu time?                   |
