DROP DATABASE IF EXISTS isma13;
CREATE DATABASE isma13;

USE isma13

CREATE TABLE jugador (
	usuario text,
	pasword text,
	jugadorID int NOT NULL AUTO_INCREMENT,
	PRIMARY KEY(jugadorID)
)ENGINE=InnoDB;

INSERT INTO jugador(usuario,pasword) VALUES ('isma','isma');
INSERT INTO jugador(usuario,pasword) VALUES ('maria','maria');
INSERT INTO jugador(usuario,pasword) VALUES ('marta','marta');
INSERT INTO jugador(usuario,pasword) VALUES ('juan','juan');

CREATE TABLE partida (
	partidaID int NOT NULL,
	duracion INT,
	ganador text,
	jugador2 text,
	PRIMARY KEY (partidaID)
)ENGINE=InnoDB;

INSERT INTO partida VALUES (1,10,'isma','marta');
INSERT INTO partida VALUES (2,15,'marta','juan');
INSERT INTO partida VALUES (3,10,'isma','marta');

CREATE TABLE relacion (
	jugadorID INT NOT NULL AUTO_INCREMENT ,
	partidaID INT NOT NULL,
	puntuacion INT,
	FOREIGN KEY (jugadorID) REFERENCES jugador(jugadorID),
	FOREIGN KEY (partidaID) REFERENCES partida(partidaID)
)ENGINE=InnoDB;

INSERT INTO relacion(partidaID,puntuacion) VALUES (1,200);
INSERT INTO relacion(partidaID,puntuacion) VALUES (2,150);
INSERT INTO relacion(partidaID,puntuacion) VALUES (3,100);
INSERT INTO relacion(partidaID,puntuacion) VALUES (1,50);
INSERT INTO relacion(partidaID,puntuacion) VALUES (4,100);
INSERT INTO relacion(partidaID,puntuacion) VALUES (1,150);

	




