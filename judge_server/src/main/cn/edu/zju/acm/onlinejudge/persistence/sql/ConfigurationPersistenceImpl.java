/*
 * Copyright (C) 2001 - 2005 ZJU Online Judge, All Rights Reserved.
 */
package cn.edu.zju.acm.onlinejudge.persistence.sql;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;


import cn.edu.zju.acm.onlinejudge.bean.Configuration;
import cn.edu.zju.acm.onlinejudge.persistence.ConfigurationPersistence;
import cn.edu.zju.acm.onlinejudge.persistence.PersistenceException;

/**
 * <p>ConfigurationPersistenceImpl implements ConfigurationPersistence interface</p>
 * <p>ConfigurationPersistence interface defines the API used to load and store Configurations
 * from the persistence layer.</p>
 *
 * @version 2.0
 * @author ZOJDEV
 */
public class ConfigurationPersistenceImpl implements ConfigurationPersistence {

	/**
	 * The query to get all configurations.
	 */
	private static final String GET_ALL_CONFIGURATIONS = "SELECT * FROM configuration";
	
	/**
	 * The query to get configuration names.
	 */
	private static final String GET_CONFIGURATION_NAMES = "SELECT name FROM configuration WHERE name IN ";
	
	/**
	 * The query to insert a configuration.
	 */
	private static final String INSERT_CONFIGURATION 
		= "INSERT INTO configuration (name, value, description, create_user, create_date, " +
				"last_update_user, last_update_date) VALUES (?, ?, ?, ?, ?, ?, ?)";
	
	/**
	 * The query to insert a configuration.
	 */
	private static final String UPDATE_CONFIGURATION 
		= "UPDATE configuration SET value=?, description=?, last_update_user=?, last_update_date=? WHERE name=?";	
		
    /**
     * <p>Returns a list of Configuration instances retrieved from persistence layer.</p>
     *
     * @return a list of Configuration instances retrieved from persistence layer.
     * @throws PersistenceException wrapping a persistence implementation specific exception
     */
    public List getConfigurations() throws PersistenceException {
    	
        Connection conn = null;
        PreparedStatement ps = null;
        ResultSet rs = null;
            	
        try {
        	conn = Database.createConnection();
            ps = conn.prepareStatement(GET_ALL_CONFIGURATIONS);            
            rs = ps.executeQuery();
            
            List configurations = new ArrayList();
            while (rs.next()) {
            	Configuration configuration = new Configuration();
            	configuration.setName(rs.getString(DatabaseConstants.CONFIGURATION_NAME));
            	configuration.setValue(rs.getString(DatabaseConstants.CONFIGURATION_VALUE));
            	configuration.setDescription(rs.getString(DatabaseConstants.CONFIGURATION_DESCRIPTION));            			
            	configurations.add(configuration);
            }            
            return configurations; 
            
        } catch (SQLException e) {
        	throw new PersistenceException("Error.", e);
		} finally {
        	Database.dispose(conn, ps, rs);
        }            	
    	
    }

    /**
     * <p>Stores the given list of Configuration instances to persistence layer.</p>
     *
     * @param configurations a list of Configuration instances to store
     * @param user the id of the user who made this modification.
     * @throws PersistenceException wrapping a persistence implementation specific exception
     * @throws NullPointerException if configurations is null
     * @throws IllegalArgumentException if configurations contains invalid or duplicate element
     */
    public void setConfigurations(List configurations, long user) throws PersistenceException {
    	if (configurations == null) {
    		throw new NullPointerException("configurations is null.");
    	}
    	
    	if (configurations.size() == 0) {
    		return;
    	}
    	
    	Set nameSet = new HashSet();
    	for (Iterator itr = configurations.iterator(); itr.hasNext();) {  
    		Object obj = itr.next();
    		if (!(obj instanceof Configuration)) {
    			throw new IllegalArgumentException("configurations contains invalid element");
    		}
    		String name = ((Configuration) obj).getName();
    		if (nameSet.contains(name)) {
    			throw new IllegalArgumentException("configurations contains duplicate element");
    		}
    		nameSet.add(name);    		
    	}
    	    	    		
    	Connection conn = null;
        PreparedStatement ps = null;
        ResultSet rs = null;
        
        try {
        	
        	conn = Database.createConnection();
        	conn.setAutoCommit(false);        	

        	// get existing configurations      	
        	ps = conn.prepareStatement(GET_CONFIGURATION_NAMES + Database.createValues(nameSet));
        	
        	rs = ps.executeQuery();
        	Set existingConfigurations = new HashSet();
        	while (rs.next()) {
        		existingConfigurations.add(rs.getString(DatabaseConstants.CONFIGURATION_NAME));        		
        	}
        	
        	// update
        	for (Iterator itr = configurations.iterator(); itr.hasNext();) { 
        		Configuration configuration = (Configuration) itr.next();
        		if (existingConfigurations.contains(configuration.getName())) {
        			ps = conn.prepareStatement(UPDATE_CONFIGURATION);
        			ps.setString(5, configuration.getName());
            		ps.setString(1, configuration.getValue());
            		ps.setString(2, configuration.getDescription());        	
            		ps.setLong(3, user);
            		ps.setTimestamp(4, new Timestamp(new Date().getTime()));            		
        		} else {
        			ps = conn.prepareStatement(INSERT_CONFIGURATION);
        			ps.setString(1, configuration.getName());
            		ps.setString(2, configuration.getValue());
            		ps.setString(3, configuration.getDescription());        	
            		ps.setLong(4, user);
            		ps.setTimestamp(5, new Timestamp(new Date().getTime()));            		
            		ps.setLong(6, user);
            		ps.setTimestamp(7, new Timestamp(new Date().getTime()));            		
        		}
        		
        		ps.executeUpdate();
        	}
        	
            conn.commit();            
        } catch (SQLException e) {
        	throw new PersistenceException("Error.", e);
		} finally {
        	Database.dispose(conn, ps, rs);
        }                       	
    }
    
}


