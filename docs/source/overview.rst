Overview
==========

How to use
----------

.. code-block:: python
  :caption: Getting the length of a GPX file

  import fastgpx

  gpx = fastgpx.parse("example.gpx")
  print(f'{gpx.length_2d()} m')


.. code-block:: python
  :caption: Iterating the tracks of a GPX file

  import fastgpx

  gpx = fastgpx.parse("example.gpx")
  for track in gpx.tracks:
      print(f'Track: {track.name}')
      print(f'Distance: {track.length_2d()} m')
      if not track.time_bounds.is_empty():
        print(f'Time: {track.time_bounds().start_time} - {track.time_bounds().end_time}')
      for segment in track.segments:
          print(f'Segment: {segment.name}')
          for point in segment.points:
              print(f'Point: {point.latitude}, {point.longitude}')


.. code-block:: python
  :caption: Encoding and decoding polylines

  import fastgpx

  locations = [
      fastgpx.LatLong(64, 10),
      fastgpx.LatLong(66, 11),
  ]
  encoded = fastgpx.polyline.encode(locations, precision=6)

  decoded = fastgpx.polyline.decode(encoded, precision=6)
