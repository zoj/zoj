<%@ page contentType="text/html; charset=utf-8" language="java" %>
<%@ taglib uri="/WEB-INF/struts-tiles.tld" prefix="tiles" %>
<%@ taglib uri="/WEB-INF/struts-bean.tld" prefix="bean" %>

<bean:define id="pageId" value="JudgeReply" toScope="request" />
<bean:define id="menuId" value="ConfigueSystem" toScope="request" />
<bean:define id="region" value="Admin" toScope="request" />

<tiles:insert definition="admin.default" flush="true" >
    <tiles:put name="title" value="ZOJ :: Admin :: Judge Reply" />
    <tiles:put name="content" value="/jsp/judge_reply_content.jsp" />
</tiles:insert>

