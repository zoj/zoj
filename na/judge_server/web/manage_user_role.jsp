<%@ page contentType="text/html; charset=utf-8" language="java" %>
<%@ taglib uri="/WEB-INF/struts-tiles.tld" prefix="tiles" %>
<%@ taglib uri="/WEB-INF/struts-bean.tld" prefix="bean" %>

<bean:define id="pageId" value="ManageUserRole" toScope="request" />
<bean:define id="menuId" value="ManageUsers" toScope="request" />
<bean:define id="region" value="Admin" toScope="request" />

<tiles:insert definition="admin.default" flush="true" >
    <tiles:put name="title" value="ZOJ :: Admin :: Manage User Role" />
    <tiles:put name="content" value="/jsp/manage_user_role_content.jsp" />
</tiles:insert>

