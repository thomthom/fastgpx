Overview
==========

How to use
----------

.. code-block:: python
  :caption: Getting the length of a GPX file

  import fastgpx

  gpx = fastgpx.load("example.gpx")
  print(f'{gpx.length_2d()} m')


.. code-block:: python
  :caption: Iterating the tracks of a GPX file

  import fastgpx

  gpx = fastgpx.load("example.gpx")
  for track in gpx.tracks:
      print(f'Track: {track.name}')
      print(f'Distance: {track.length_2d()} m')
      time_bounds = track.time_bounds()
      if not time_bounds.is_empty():
          print(f'Time: {time_bounds.start_time} - {time_bounds.end_time}')
      for segment in track.segments:
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


.. code-block:: python
  :caption: Handling errors

  import fastgpx

  try:
      gpx = fastgpx.load("example.gpx")
      time_bounds = gpx.time_bounds()
  except fastgpx.ParseError as error:
      print(f"Malformed GPX: {error}")
  except OSError as error:
      print(f"Cannot read file: {error}")


Changing a point
----------------

Indexing or iterating ``segment.points`` gives copies of the points, and slicing it gives a new,
independent list, so assigning to an attribute of a point does not change the segment. Assign
the point back instead:

.. code-block:: python
  :caption: Moving the first point of a segment

  import fastgpx

  gpx = fastgpx.load("example.gpx")
  segment = gpx.tracks[0].segments[0]

  point = segment.points[0]
  point.latitude = 60.0
  segment.points[0] = point

