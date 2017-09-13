CREATE TABLE IF NOT EXISTS
quicksv_table(
  code varchar(50) not null primary key,
  zipcode varchar(10),
  town varchar(255),
  streetname varchar(255),
  housenumber varchar(255),
  sortrow varchar(50),
  sortbox varchar(50),
  ocrtext varchar(4000),
  customerno varchar(20),
  costcenter varchar(20),
  createtime datetime
);

DROP PROCEDURE IF EXISTS quicksv;
DELIMITER //
CREATE PROCEDURE quicksv
(
  IN in_code varchar(50),
  IN in_zipcode varchar(10),
  IN in_town varchar(255),
  IN in_streetname varchar(255),
  IN in_housenumber varchar(255),
  IN in_sortrow varchar(50),
  IN in_sortbox varchar(50),
  IN in_string varchar(4000),
  IN in_customerno varchar(20),
  IN in_costcenter varchar(20)
)
MODIFIES SQL DATA
BEGIN
  insert into quicksv_table(
    code,
    zipcode,
    town,
    streetname,
    housenumber,
    sortrow,
    sortbox,
    ocrtext,
    customerno,
    costcenter,
    createtime
  ) values(
    in_code,
    in_zipcode,
    in_town,
    in_streetname,
    in_housenumber,
    in_sortrow,
    in_sortbox,
    in_string,
    in_customerno,
    in_costcenter,
    now()
  )
  on duplicate key update
  zipcode=values(zipcode),
  town=values(town),
  streetname=values(streetname),
  housenumber=values(housenumber),
  sortrow=values(sortrow),
  sortbox=values(sortbox),
  ocrtext=values(ocrtext),
  customerno=values(customerno),
  costcenter=values(costcenter);

END;
//
DELIMITER ;
