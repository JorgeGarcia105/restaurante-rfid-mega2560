```markdown
 # PROYECTO MICROPROCESADORES

Realizar la programación de un dispositivo para tomar los pedidos en un restaurante, con las
siguientes condiciones:

- Toda la información que deba mostrarse al usuario se hará por medio de un LCD.
- El cliente puede seleccionar entre las siguientes opciones: entrada, plato fuerte, bebida y
	postre. Cada una de éstas, tendrá varias opciones, incluido la opción de no seleccionar
	nada en alguna de ellas o seleccionar varias (en el caso de que sean varias personas en la
	mesa).
- El dispositivo debe permitir tomar pedidos de varios clientes y debe totalizar el costo del
	pedido de cada cliente.
- Cada pedido debe ser enviado a través de comunicación serial a la cocina, donde
	dispondrán de dos botones para avisar que el pedido está listo. Uno de los botones se
	usará para seleccionar el pedido y el otro para indicar que está listo. Esta información se
	visualizará en displays de 7 segmentos (número de pedido) y en un led (estilo alarma) para
	que el cliente pueda observarla. Además se comunicará a través del serial al dispositivo,
	para que el mesero recoja el pedido y lo lleve a la mesa. Como se debe manejar una lista
	de comandas (esta es la lista enviada a la cocina), cada vez que un pedido es entregado, se
	debe borrar de la lista de comandas. Esta lista es distinta de la de pedidos, en la que solo
	puede desaparecer un pedido, una vez que el cliente lo ha pagado.
- El cliente pagará usando una tarjeta RFID. El dispositivo deberá contar con un modo para
	recargar la tarjeta con saldo y cada vez que el cliente la use para pagar, deberá descontar
	el total del costo del pedido de ella.
```
