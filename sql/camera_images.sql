create table camera_images (
  id varchar(25) not null primary key,
  inserttime datetime not null,
  kunde varchar(20) not null,
  state varchar(5) not null
);


create table camera_imagescodes(
  id varchar(25) not null,
  code varchar(50) not null,
  primary key (id,code),

  constraint `fk_camera_imagescodes_camera_images_id`
  foreign key (`id`)
  references camera_images(id)
  on delete cascade 
  on update cascade

);