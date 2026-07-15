import console from 'console';

import { EMPTY_OBJECT, shallowClonePojo } from 'sequelize/utils';


import { Sequelize } from 'sequelize/core';
console.info("start testing sequlize");
const sequelize = new Sequelize('sqlite::memory:');
/* const User = sequelize.define('User', {
	username: DataTypes.STRING,
	birthday: DataTypes.DATE,
}); */